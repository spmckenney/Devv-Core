/*
 * Tier2Transaction.h defines the structure of the transaction section of a block
 * for Tier 2 shards.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 *
 *
 */

#ifndef PRIMITIVES_TIER2TRANSACTION_H_
#define PRIMITIVES_TIER2TRANSACTION_H_

#include "Transaction.h"

using namespace Devcash;

namespace Devcash {

/**
 * The Tier2 representation of a Transaction
 */
class Tier2Transaction : public Transaction {
 public:
  /**
   * Move constructor
   * @param other
   */
   Tier2Transaction(Tier2Transaction&& other) = default;

  /**
   * Default move assignment constructor
   * @param other
   */
   Tier2Transaction& operator=(Tier2Transaction&& other) = default;

  /**
   * Constructor
   * @param oper Operation of this transaction
   * @param xfers
   * @param nonce
   * @param eckey
   * @param keys
   */
  Tier2Transaction(byte oper,
                   const std::vector<Transfer>& xfers,
                   uint64_t nonce,
                   EC_KEY* eckey,
                   const KeyRing& keys)
      : Transaction(xfers.size(), false) {
    canonical_.reserve(MinSize() + (Transfer::Size() * xfer_count_));

    Uint64ToBin(xfer_count_, canonical_);
    canonical_.push_back(oper);
    for (auto& transfer : xfers) {
      std::vector<byte> xfer_canon(transfer.getCanonical());
      canonical_.insert(std::end(canonical_), std::begin(xfer_canon), std::end(xfer_canon));
    }
    Uint64ToBin(nonce, canonical_);
    std::vector<byte> msg(getMessageDigest());
    Signature sig;
    SignBinary(eckey, DevcashHash(msg), sig);
    canonical_.insert(std::end(canonical_), std::begin(sig), std::end(sig));
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T2 transaction, not sound!";
    }
  }

  static Tier2Transaction Create(InputBuffer& buffer,
                                 const KeyRing& keys,
                                 bool calculate_soundness = true);

  static std::unique_ptr<Tier2Transaction> CreateUniquePtr(InputBuffer& buffer,
                                                           const KeyRing& keys,
                                                           bool calculate_soundness = true);

  /**
   * Creates a clone (deep copy) of this transaction
   * @return
   */
  std::unique_ptr<Transaction> clone() const override {
    return std::unique_ptr<Transaction>(new Tier2Transaction(*this));
  }

  /// Declare make_unique<>() as a friend
  friend std::unique_ptr<Tier2Transaction> std::make_unique<Tier2Transaction>();

 private:
  /**
   * Private default constructor
   */
  Tier2Transaction() = default;

  /**
   * Default private copy constructor. We keep this private so the programmer has to
   * explicity call clone() to avoid accidental copies
   */
   Tier2Transaction(const Tier2Transaction& other) = default;

   /**
    * Fill the given Tier2Transaction from the InputBuffer
    * @param tx
    * @param buffer
    * @param keys
    * @param calculate_soundness
    */
   static void Fill(Tier2Transaction& tx,
                    InputBuffer &buffer,
                    const KeyRing &keys,
                    bool calculate_soundness);
  /**
   * Return a copy of the message digest
   * @return message digest
   */
  std::vector<byte> do_getMessageDigest() const override {
    /// @todo(mckenney) can this be a reference?
    std::vector<byte> md(canonical_.begin(), canonical_.begin() + (EnvelopeSize() + Transfer::Size() * xfer_count_));
    return md;
  }

  /**
   * Returns the operation type of this transaction
   * @return byte representation of this operation
   */
  byte do_getOperation() const override { return canonical_[8]; }

  /**
   * Create and return a vector of Transfers in this transaction
   * @return vector of Transfer objects
   */
  std::vector<Transfer> do_getTransfers() const {
    std::vector<Transfer> out;
    /// @todo - Hardcoded value
    InputBuffer buffer(canonical_, 9);
    for (size_t i = 0; i < xfer_count_; ++i) {
      /// @todo memory leak!!
      Transfer* t = new Transfer(buffer);
      out.push_back(*t);
    }
    return out;
  }

  /**
   * Return the nonce of this transaction
   * @return the nonce
   */
  uint64_t do_getNonce() const {
    /// @todo hard-coded value
    return BinToUint64(canonical_, 9 + (Transfer::Size() * xfer_count_));
  }

  /**
   * Create and return the signature of this transaction
   * @return the signature of this transaction
   */
  Signature do_getSignature() const {
    /// @todo(mckenney) can this be a reference rather than pointer?
    Signature sig;
    std::copy_n(canonical_.begin() + (17 + (Transfer::Size() * xfer_count_)), kSIG_SIZE, sig.begin());
    return sig;
  }

  /**
   * Compute and set the soundness
   * @param keys KeyRing to use to check soundness
   * @return true if this Transaction is sound
   */
  bool do_setIsSound(const KeyRing& keys) {
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized transaction, not sound!";
    }
    return is_sound_;
  }

  /**
   * Return soundness. If is_sound_ is not true, calculate soundness
   * @param keys KeyRing to use to compute soundness
   * @return true if sound
   */
  bool do_isSound(const KeyRing& keys) const {
    MTR_SCOPE_FUNC();
    //CASH_TRY {
      if (is_sound_) return (is_sound_);
      long total = 0;
      byte oper = getOperation();

      if (getNonce() < 1) {
        LOG_WARNING << "Error: nonce is required";
        return (false);
      }

      bool sender_set = false;
      Address sender;

      std::vector<Transfer> xfers = getTransfers();
      for (auto it = xfers.begin(); it != xfers.end(); ++it) {
        int64_t amount = it->getAmount();
        total += amount;
        if ((oper == eOpType::Delete && amount > 0) || (oper != eOpType::Delete && amount < 0) ||
            oper == eOpType::Modify) {
        }
        if (amount < 0) {
          if (sender_set) {
            LOG_WARNING << "Multiple senders in transaction!";
            return false;
          }
          sender = it->getAddress();
          sender_set = true;
        }
      }
      if (total != 0) {
        LOG_WARNING << "Error: transaction amounts are asymmetric. (sum=" + std::to_string(total) + ")";
        return false;
      }

      if ((oper != Exchange) && (!keys.isINN(sender))) {
        LOG_WARNING << "INN transaction not performed by INN!";
        return false;
      }

      EC_KEY* eckey(keys.getKey(sender));
      std::vector<byte> msg(getMessageDigest());
      Signature sig = getSignature();

      if (!VerifyByteSig(eckey, DevcashHash(msg), sig)) {
        LOG_WARNING << "Error: transaction signature did not validate.\n";
        LOG_DEBUG << "Transaction state is: " + getJSON();
        LOG_DEBUG << "Sender addr is: " + ToHex(std::vector<byte>(std::begin(sender), std::end(sender)));
        LOG_DEBUG << "Signature is: " + ToHex(std::vector<byte>(std::begin(sig), std::end(sig)));
        return false;
      }
      return true;
    //}
    //CASH_CATCH(const std::exception& e) { LOG_WARNING << FormatException(&e, "transaction"); }
    return false;
  }

  /**
   * Compute whether or not this Transaction is valid
   * @param state
   * @param keys
   * @param summary
   * @return
   */
  bool do_isValid(ChainState& state, const KeyRing& keys, Summary& summary) const override {
    CASH_TRY {
      if (!isSound(keys)) return false;
      byte oper = getOperation();
      std::vector<Transfer> xfers = getTransfers();
      for (auto it = xfers.begin(); it != xfers.end(); ++it) {
        int64_t amount = it->getAmount();
        uint64_t coin = it->getCoin();
        Address addr = it->getAddress();
        if (amount < 0) {
          if ((oper == Exchange) && (amount > state.getAmount(coin, addr))) {
            LOG_WARNING << "Coins not available at addr.";
            return false;
          }
        }
        SmartCoin next_flow(addr, coin, amount);
        state.addCoin(next_flow);
        summary.addItem(addr, coin, amount, it->getDelay());
      }
      return true;
    }
    CASH_CATCH(const std::exception& e) { LOG_WARNING << FormatException(&e, "transaction"); }
    return false;
  }

  /*
  std::map<Address, SmartCoin> do_aggregateState(std::map<Address, SmartCoin>& aggregator, const ChainState& state,
                                                 const KeyRing& keys, const Summary&) const override {
    CASH_TRY {
      if (!isSound(keys)) return aggregator;
      byte oper = getOperation();
      std::vector<Transfer> xfers = getTransfers();
      for (auto it = xfers.begin(); it != xfers.end(); ++it) {
        int64_t amount = it->getAmount();
        uint64_t coin = it->getCoin();
        Address addr = it->getAddress();
        if (amount < 0) {
          if ((oper == Exchange) && (amount > state.getAmount(coin, addr))) {
            LOG_WARNING << "Coins not available at addr.";
            return aggregator;
          }
        }
        SmartCoin next_flow(addr, coin, amount);
        auto loc = aggregator.find(addr);
        if (loc == aggregator.end()) {
          std::pair<Address, SmartCoin> pair(addr, next_flow);
          aggregator.insert(pair);
        } else {
          loc->second.amount_ += amount;
        }
      }
    }
    CASH_CATCH(const std::exception& e) { LOG_WARNING << FormatException(&e, "transaction"); }
    return aggregator;
  }
  */

  /**
   * Returns a JSON string representing this transaction.
   * @return a JSON string representing this transaction.
   */
  std::string do_getJSON() const {
    std::string json("{\"" + kXFER_COUNT_TAG + "\":");
    json += std::to_string(xfer_count_) + ",";
    json += "\"" + kOPER_TAG + "\":" + std::to_string(getOperation()) + ",";
    json += "\"" + kXFER_TAG + "\":[";
    bool isFirst = true;
    std::vector<Transfer> xfers = getTransfers();
    for (auto it = xfers.begin(); it != xfers.end(); ++it) {
      if (isFirst) {
        isFirst = false;
      } else {
        json += ",";
      }
      json += it->getJSON();
    }
    json += "],\"" + kNONCE_TAG + "\":" + std::to_string(getNonce()) + ",";
    Signature sig = getSignature();
    json += "\"" + kSIG_TAG + "\":\"" + ToHex(std::vector<byte>(std::begin(sig), std::end(sig))) + "\"}";
    return json;
  }
};

typedef std::unique_ptr<Tier2Transaction> Tier2TransactionPtr;
}  // end namespace Devcash

#endif  // DEVCASH_PRIMITIVES_TRANSACTION_H
