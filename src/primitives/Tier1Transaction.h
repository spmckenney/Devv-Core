/*
 * Tier1Transaction.h defines the structure of the transaction section of a block
 * for Tier 1 shards.
 *
 *  Created on: May 4, 2018
 *  Author: Nick Williams
 *
 */

#ifndef PRIMITIVES_TIER1TRANSACTION_H_
#define PRIMITIVES_TIER1TRANSACTION_H_

#include "Transaction.h"

using namespace Devcash;

namespace Devcash {

class Tier1Transaction : public Transaction {
 public:
  // This is to support the unimplemented oracleInterface::getT1Syntax() methods
  Tier1Transaction() = default;

  /**
   * Constructor
   * @param serial byte representation of this transaction
   * @param keys KeyRing
   */
  explicit Tier1Transaction(const std::vector<byte>& serial, const KeyRing& keys) : sum_size_(0) {
    MTR_SCOPE_FUNC();
    int trace_int = 124;
    MTR_START("Transaction", "Transaction", &trace_int);
    MTR_STEP("Transaction", "Transaction", &trace_int, "step1");

    canonical_.insert(canonical_.end(), serial.begin(), serial.begin());
    size_t offset = 8;

    canonical_.insert(canonical_.end(), serial.begin() + offset, serial.begin() + 8);
    sum_size_ = BinToUint64(serial, offset);
    offset += 8;

    MTR_STEP("Transaction", "Transaction", &trace_int, "step2");
    if (serial.size() < offset + sum_size_ + kSIG_SIZE) {
      LOG_WARNING << "Invalid serialized T1 transaction, too small!";
      return;
    }

    canonical_.insert(canonical_.end(), serial.begin() + offset, serial.begin() + sum_size_ + kSIG_SIZE);
    offset += sum_size_ + kSIG_SIZE;

    MTR_STEP("Transaction", "Transaction", &trace_int, "sound");
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
    }
    MTR_FINISH("Transaction", "Transaction", &trace_int);
  }

  /**
   * Constructor
   * @param[in] serial The serialized buffer to construction this transaction from
   * @param[in,out] offset The pointer to the serialized buffer location.
   *                       Will get incremented by the size of this transaction.
   * @param[in] keys a KeyRing that provides keys for signature verification
   * @param[in] calculate_soundness if true will perform soundness check on this transaction
   */
  explicit Tier1Transaction(const std::vector<byte>& serial,
                            size_t& offset,
                            const KeyRing& keys,
                            bool calculate_soundness = true)
      : sum_size_(0) {
    MTR_SCOPE_FUNC();
    int trace_int = 124;
    MTR_START("Transaction", "Transaction", &trace_int);
    MTR_STEP("Transaction", "Transaction", &trace_int, "step1");

    canonical_.insert(canonical_.end(), serial.begin() + offset, serial.begin() + offset + 8);
    offset += 8;

    canonical_.insert(canonical_.end(), serial.begin() + offset, serial.begin() + offset + 8);
    sum_size_ = BinToUint64(serial, offset);
    offset += 8;

    MTR_STEP("Transaction", "Transaction", &trace_int, "step2");
    if (serial.size() < offset + sum_size_ + kSIG_SIZE) {
      LOG_WARNING << "Invalid serialized T1 transaction, too small!";
      return;
    }

    canonical_.insert(canonical_.end(), serial.begin() + offset, serial.begin() + offset + sum_size_ + kSIG_SIZE);
    offset += sum_size_ + kSIG_SIZE;

    MTR_STEP("Transaction", "Transaction", &trace_int, "sound");
    if (calculate_soundness) {
      is_sound_ = isSound(keys);
      if (!is_sound_) {
        LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
      }
    }
    MTR_FINISH("Transaction", "Transaction", &trace_int);
  }

  /**
   * Constructor
   * @param[in] summary Transaction summary
   * @param[in] sig Transaction signature
   * @param[in] nonce nonce
   * @param[in] keys KeyRing to use for soundness
   */
  Tier1Transaction(const Summary& summary,
                   const Signature& sig,
                   uint64_t nonce,
                   const KeyRing& keys)
      : Transaction(0, false), sum_size_(summary.getByteSize()) {
    if (!summary.isSane()) {
      LOG_WARNING << "Serialized T1 transaction has bad summary!";
    }
    xfer_count_ = summary.getTransferCount();
    canonical_.reserve(sum_size_ + 16 + kSIG_SIZE);
    Uint64ToBin(nonce, canonical_);
    Uint64ToBin(sum_size_, canonical_);
    std::vector<byte> sum_canon(summary.getCanonical());
    canonical_.insert(std::end(canonical_), std::begin(sum_canon), std::end(sum_canon));
    canonical_.insert(std::end(canonical_), std::begin(sig), std::end(sig));
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
    }
  }

  /**
   * Creates a deep copy of this transaction
   * @return
   */
  std::unique_ptr<Transaction> clone() const override {
    return std::unique_ptr<Transaction>(new Tier1Transaction(*this));
  }

 private:
  /**
   * Return a copy of the message digest
   * @return vector of bytes
   */
  std::vector<byte> do_getMessageDigest() const {
    /// @todo(spm) does this have to be a copy?
    std::vector<byte> md(canonical_.begin() + 16, canonical_.begin() + sum_size_ + 16);
    return md;
  }

  /**
   * Returns 0 - doesn't apply to Tier1Transactions
   * @return
   */
  byte do_getOperation() const override { return (byte)0; }

  /**
   * Gets a vector of Transfers
   * @return vector of Transfer objects
   */
  std::vector<Transfer> do_getTransfers() const {
    /// @todo Address hard-coded value
    size_t offset = 16;
    Summary summary(canonical_, offset);
    return summary.getTransfers();
  }

  /**
   * Get the nonce of this Transaction
   * @return nonce
   */
  uint64_t do_getNonce() const { return BinToUint64(canonical_, 0); }

  /**
   * Get a copy of the Signature of this Transaction
   * @return Signature
   */
  Signature do_getSignature() const {
    /// @todo(spm) Does this have to be a copy?
    Signature sig;
    std::copy_n(canonical_.begin() + sum_size_ + 16, kSIG_SIZE, sig.begin());
    return sig;
  }

  /**
   * Compute and set the soundness
   * @param[in] keys KeyRing to check soundness
   * @return
   */
  bool do_setIsSound(const KeyRing& keys) {
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
    }
    return is_sound_;
  }

  /** Checks if this transaction is sound, meaning potentially valid.
   *  If any portion of the transaction is invalid,
   *  the entire transaction is also unsound.
   * @params keys a KeyRing that provides keys for signature verification
   * @return true iff the transaction is sound
   * @return false otherwise
   */
  bool do_isSound(const KeyRing& keys) const {
    MTR_SCOPE_FUNC();
    CASH_TRY {
      if (is_sound_) return (is_sound_);

      /// @todo(mckenney) auto node_index
      int node_index = getNonce();
      EC_KEY* eckey(keys.getNodeKey(node_index));
      std::vector<byte> msg(getMessageDigest());
      Signature sig = getSignature();

      if (!VerifyByteSig(eckey, DevcashHash(msg), sig)) {
        LOG_WARNING << "Error: T1 transaction signature did not validate.\n";
        LOG_DEBUG << "Transaction state is: " + getJSON();
        LOG_DEBUG << "Node index is: " + std::to_string(node_index);
        LOG_DEBUG << "Signature is: " + ToHex(std::vector<byte>(std::begin(sig), std::end(sig)));
        return false;
      }
      return true;
    }
    CASH_CATCH(const std::exception& e) { LOG_WARNING << FormatException(&e, "transaction"); }
    return false;
  }

  /** Checks if this transaction is valid with respect to a chain state.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   * @params state the chain state to validate against
   * @params keys a KeyRing that provides keys for signature verification
   * @params summary the Summary to update
   * @return true iff the transaction is valid
   * @return false otherwise
   */
  bool do_isValid(ChainState& state, const KeyRing& keys, Summary& summary) const override {
    CASH_TRY {
      if (!isSound(keys)) return false;

      std::vector<Transfer> xfers = getTransfers();
      for (auto& transfer : xfers) {
        int64_t amount = transfer.getAmount();
        uint64_t coin = transfer.getCoin();
        Address addr = transfer.getAddress();
        SmartCoin next_flow(addr, coin, amount);
        state.addCoin(next_flow);
        summary.addItem(addr, coin, amount, transfer.getDelay());
      }
      return true;
    }
    CASH_CATCH(const std::exception& e) { LOG_WARNING << FormatException(&e, "transaction"); }
    return false;
  }

  /*
   std::map<Address, SmartCoin> do_aggregateState(std::map<Address, SmartCoin>& aggregator, const ChainState&,
                                                 const KeyRing& keys, const Summary&) const override {
    CASH_TRY {
      if (!isSound(keys)) return aggregator;

      std::vector<Transfer> xfers = getTransfers();
      for (auto it = xfers.begin(); it != xfers.end(); ++it) {
        int64_t amount = it->getAmount();
        uint64_t coin = it->getCoin();
        Address addr = it->getAddress();
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
    for (auto& transfer : xfers) {
      if (isFirst) {
        isFirst = false;
      } else {
        json += ",";
      }
      json += transfer.getJSON();
    }
    json += "],\"" + kNONCE_TAG + "\":" + std::to_string(getNonce()) + ",";
    Signature sig = getSignature();
    json += "\"" + kSIG_TAG + "\":\"" + ToHex(std::vector<byte>(std::begin(sig), std::end(sig))) + "\"}";
    return json;
  }

  uint64_t sum_size_;
};

typedef std::unique_ptr<Tier1Transaction> Tier1TransactionPtr;

}  // end namespace Devcash

#endif /* PRIMITIVES_TIER1TRANSACTION_H_ */
