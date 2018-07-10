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
#include "primitives/buffers.h"

using namespace Devcash;

namespace Devcash {

class Tier1Transaction : public Transaction {
 public:
  /**
   * Constructor
   * @param[in] serial The serialized buffer to construction this transaction from
   * @param[in,out] offset The pointer to the serialized buffer location.
   *                       Will get incremented by the size of this transaction.
   * @param[in] keys a KeyRing that provides keys for signature verification
   * @param[in] calculate_soundness if true will perform soundness check on this transaction
   */
  explicit Tier1Transaction(InputBuffer& buffer,
                            const KeyRing& keys,
                            bool calculate_soundness = true)
      : sum_size_(0) {
    MTR_SCOPE_FUNC();
    int trace_int = 124;
    MTR_START("Transaction", "Transaction", &trace_int);
    MTR_STEP("Transaction", "Transaction", &trace_int, "step1");

    sum_size_ = buffer.getNextUint64(false);
    buffer.copy(std::back_inserter(canonical_)
      , sum_size_ + kSIG_SIZE + uint64Size()*2);

    MTR_STEP("Transaction", "Transaction", &trace_int, "step2");
    if (buffer.size() < buffer.getOffset()
                        + sum_size_ + kSIG_SIZE + uint64Size()*2) {
      LOG_WARNING << "Invalid serialized T1 transaction, too small!";
      return;
    }

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
                   uint64_t node_dex,
                   const KeyRing& keys)
      : Transaction(false), sum_size_(summary.getByteSize()) {
    if (!summary.isSane()) {
      LOG_WARNING << "Serialized T1 transaction has bad summary!";
    }
    sum_size_ = summary.getByteSize();
    canonical_.reserve(sum_size_ + uint64Size()*2 + kSIG_SIZE);
    Uint64ToBin(sum_size_, canonical_);
    std::vector<byte> sum_canon(summary.getCanonical());
    canonical_.insert(std::end(canonical_), std::begin(sum_canon), std::end(sum_canon));
    Uint64ToBin(node_dex, canonical_);
    canonical_.insert(std::end(canonical_), std::begin(sig), std::end(sig));
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
    }
  }

  static Tier1Transaction Create() {
    Tier1Transaction tx;
    return(tx);
  };

  static Tier1Transaction Create(InputBuffer& buffer,
                                 const KeyRing& keys,
                                 bool calculate_soundness = true);

  static std::unique_ptr<Tier1Transaction> CreateUniquePtr(InputBuffer& buffer,
                                                           const KeyRing& keys,
                                                           bool calculate_soundness = true);

  /**
   * Creates a deep copy of this transaction
   * @return
   */
  std::unique_ptr<Transaction> clone() const override {
    return std::unique_ptr<Transaction>(new Tier1Transaction(*this));
  }

  /// Declare make_unique<>() as a friend
  friend std::unique_ptr<Tier1Transaction> std::make_unique<Tier1Transaction>();

  /**
   * Get the node index of the T2 validator that signed this transaction.
   * @return node index
   */
  uint64_t getNodeIndex() const {
    size_t offset = transferOffset();
    return BinToUint64(canonical_, offset+sum_size_+uint64Size());
  }

 private:
  //The size of this Transaction's canonical summary
  uint64_t sum_size_ = 0;

  /**
   * Private default constructor
   */
  Tier1Transaction() = default;

  static void Fill(Tier1Transaction& tx,
                   InputBuffer &buffer,
                   const KeyRing &keys,
                   bool calculate_soundness);

  /**
   * Return a copy of the message digest
   * @return vector of bytes
   */
  std::vector<byte> do_getMessageDigest() const {
    /// @todo(spm) does this have to be a copy?
    std::vector<byte> md(canonical_.begin() + uint64Size()
      , canonical_.begin() + sum_size_ + uint64Size());
    return md;
  }

  /**
   * Gets a vector of Transfers
   * @return vector of Transfer objects
   */
  std::vector<TransferPtr> do_getTransfers() const {
    size_t offset = transferOffset();
    InputBuffer buffer(canonical_, offset);
    Summary summary(Summary::Create(buffer));
    return summary.getTransfers();
  }

  /**
   * Get a copy of the Signature of this Transaction
   * @return Signature
   */
  Signature do_getSignature() const {
    //Signature should be immutable so copy on request
    Signature sig;
    std::copy_n(canonical_.begin() + sum_size_ + uint64Size()*2
      , kSIG_SIZE, sig.begin());
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
  bool do_isSound(const KeyRing& keys) const override {
    MTR_SCOPE_FUNC();
    try {
      if (is_sound_) { return (is_sound_); }

      int node_index = getNodeIndex();
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
    } catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "transaction");
    }
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
    try {
      if (!isSound(keys)) { return false; }

      std::vector<TransferPtr> xfers = getTransfers();
      for (auto& transfer : xfers) {
        int64_t amount = transfer->getAmount();
        uint64_t coin = transfer->getCoin();
        Address addr = transfer->getAddress();
        SmartCoin next_flow(addr, coin, amount);
        state.addCoin(next_flow);
        summary.addItem(addr, coin, amount, transfer->getDelay());
      }
      return true;
    }
    catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "transaction");
    }
    return false;
  }

  /**
   * Returns a JSON string representing this transaction.
   * @return a JSON string representing this transaction.
   */
  std::string do_getJSON() const {
    size_t offset = uint64Size();
    InputBuffer buffer(canonical_, offset);
    Summary summary(Summary::Create(buffer));
    std::string json("{\"" + kSUM_SIZE_TAG + "\":");
    json += std::to_string(sum_size_) + ",";
    json += "\"" + kSUMMARY_TAG + "\":[";
    json += "{\"" + kADDR_SIZE_TAG + "\":";
    auto summary_map = summary.getSummaryMap();
    uint64_t addr_size = summary_map.size();
    json += std::to_string(addr_size) + ",\""+kSUMMARY_TAG+"\":[";
    for (auto summary : summary_map) {
      json += "\"" + ToHex(std::vector<byte>(std::begin(summary.first)
        , std::end(summary.first))) + "\":[";
      SummaryPair top_pair(summary.second);
      DelayedMap delayed(top_pair.first);
      CoinMap coin_map(top_pair.second);
      uint64_t delayed_size = delayed.size();
      uint64_t coin_size = coin_map.size();
      json += "\"" + kDELAY_SIZE_TAG + "\":" + std::to_string(delayed_size) + ",";
      json += "\"" + kCOIN_SIZE_TAG + "\":" + std::to_string(coin_size) + ",";
      bool is_first = true;
      json += "\"delayed\":[";
      for (auto delayed_item : delayed) {
        if (is_first) {
          is_first = false;
        } else {
          json += ",";
        }
        json += "\"" + kTYPE_TAG + "\":" + std::to_string(delayed_item.first) + ",";
        json += "\"" + kDELAY_TAG + "\":" + std::to_string(delayed_item.second.delay) + ",";
        json += "\"" + kAMOUNT_TAG + "\":" + std::to_string(delayed_item.second.delta);
      }
      json += "],\"coin_map\":[";
      is_first = true;
      for (auto coin : coin_map) {
        if (is_first) {
          is_first = false;
        } else {
          json += ",";
        }
        json += "\"" + kTYPE_TAG + "\":" + std::to_string(coin.first) + ",";
        json += "\"" + kAMOUNT_TAG + "\":" + std::to_string(coin.second);
      }
      json += "]";
    }
    json += "]}";
    json += "],\"" + kVALIDATOR_DEX_TAG + "\":" + std::to_string(getNodeIndex()) + ",";
    Signature sig = getSignature();
    json += "\"" + kSIG_TAG + "\":\"" + ToHex(std::vector<byte>(std::begin(sig), std::end(sig))) + "\"}";
    return json;
  }

};

typedef std::unique_ptr<Tier1Transaction> Tier1TransactionPtr;
}  // end namespace Devcash

#endif /* PRIMITIVES_TIER1TRANSACTION_H_ */
