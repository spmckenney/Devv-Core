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
                   std::vector<Transfer>& xfers,
                   std::vector<byte> nonce,
                   EC_KEY* eckey,
                   const KeyRing& keys)
      : Transaction(false) {
    nonce_size_ = nonce.size();

    if (nonce_size_ < minNonceSize()) {
      LOG_WARNING << "Invalid serialized T2 transaction, nonce is too small ("
        +std::to_string(nonce_size_)+").";
    }

    Uint64ToBin(nonce_size_, canonical_);
    canonical_.push_back(oper);

    size_t xfer_size = 0;
    for (auto& transfer : xfers) {
      xfer_size += transfer.Size();
      std::vector<byte> xfer_canon(transfer.getCanonical());
      canonical_.insert(std::end(canonical_), std::begin(xfer_canon), std::end(xfer_canon));
    }
    xfer_size_ = xfer_size;

    std::vector<byte> xfer_size_bin;
    Uint64ToBin(xfer_size_, xfer_size_bin);
    //Note that xfer size goes on the beginning
    canonical_.insert(std::begin(canonical_), std::begin(xfer_size_bin), std::end(xfer_size_bin));

    canonical_.insert(std::end(canonical_), std::begin(nonce), std::end(nonce));
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
   * Returns the operation performed by this transfer
   * @return
   */
  byte getOperation() const { return canonical_[16]; }

  /**
   * Get the nonce of this Transaction
   * @return
   */
  std::vector<byte> getNonce() const {
	std::vector<byte> nonce(canonical_.begin() + (EnvelopeSize() + xfer_size_)
	  , canonical_.begin() + (EnvelopeSize() + xfer_size_ + nonce_size_));
    return nonce;
  }

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
  // The bytesize of Transfers in this Transaction
  uint64_t xfer_size_ = 0;
  // The size of this Transaction's nonce
  uint64_t nonce_size_ = 0;

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
    std::vector<byte> md(canonical_.begin(), canonical_.begin()
      + (EnvelopeSize() + nonce_size_ + xfer_size_));
    return md;
  }

  /**
   * Create and return a vector of Transfers in this transaction
   * @return vector of Transfer objects
   */
  std::vector<TransferPtr> do_getTransfers() const {
    std::vector<TransferPtr> out;
    size_t offset = 0;
    while (offset < xfer_size_) {
      InputBuffer buffer(canonical_, transferOffset() + offset);
      TransferPtr t = std::make_unique<Transfer>(buffer);
      offset += t->Size();
      out.push_back(std::move(t));
    }
    return out;
  }

  /**
   * Create and return the signature of this transaction
   * @return the signature of this transaction
   */
  Signature do_getSignature() const {
    /// @todo(mckenney) can this be a reference rather than pointer?
    Signature sig;
    std::copy_n(canonical_.begin() + (EnvelopeSize() + nonce_size_ + xfer_size_)
      , kSIG_SIZE, sig.begin());
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
    try {
      if (is_sound_) { return (is_sound_); }
      long total = 0;
      byte oper = getOperation();

      bool sender_set = false;
      Address sender;

      std::vector<TransferPtr> xfers = getTransfers();
      for (auto& it : xfers) {
        int64_t amount = it->getAmount();
        total += amount;
        if (amount < 0) {
          if (sender_set) {
            if (sender != it->getAddress()) {
              LOG_WARNING << "Multiple senders in transaction!";
              return false;
            } else {
              LOG_INFO << "Sending multiple distinct transfers at once.";
            }
          }
          sender = it->getAddress();
          sender_set = true;
        }
      }
      if (oper == eOpType::Delete || oper == eOpType::Modify) {
        if (!keys.isINN(sender)) {
          LOG_WARNING << "Invalid operation, non-INN address deleting or modifying coins.";
          return false;
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
        LOG_DEBUG << "Sender addr is: " + sender.getJSON();
        LOG_DEBUG << "Signature is: " + ToHex(std::vector<byte>(std::begin(sig), std::end(sig)));
        return false;
      }
      return true;
    } catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "transaction");
    }
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
      if (!isSound(keys)) { return false; }
      byte oper = getOperation();
      std::vector<TransferPtr> xfers = getTransfers();
      for (auto& it : xfers) {
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

  /**
   * Returns a JSON string representing this transaction.
   * @return a JSON string representing this transaction.
   */
  std::string do_getJSON() const {
    std::string json("{\"" + kXFER_SIZE_TAG + "\":");
    json += std::to_string(xfer_size_) + ",";
    json += "\"" + kNONCE_SIZE_TAG + "\":"+std::to_string(nonce_size_)+",";
    json += "\"" + kOPER_TAG + "\":" + std::to_string(getOperation()) + ",";
    json += "\"" + kXFER_TAG + "\":[";
    bool isFirst = true;
    std::vector<TransferPtr> xfers = getTransfers();
    for (auto& it : xfers) {
      if (isFirst) {
        isFirst = false;
      } else {
        json += ",";
      }
      json += it->getJSON();
    }
    json += "],\"" + kNONCE_TAG + "\":\"" + ToHex(getNonce()) + "\",";
    Signature sig = getSignature();
    json += "\"" + kSIG_TAG + "\":\"" + ToHex(std::vector<byte>(std::begin(sig), std::end(sig))) + "\"}";
    return json;
  }
};

typedef std::unique_ptr<Tier2Transaction> Tier2TransactionPtr;
}  // end namespace Devcash

#endif  // DEVCASH_PRIMITIVES_TRANSACTION_H
