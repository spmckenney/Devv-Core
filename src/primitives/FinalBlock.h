/*
 * FinalBlock.h
 *
 *  Created on: Apr 21, 2018
 *      Author: Nick Williams
 */

#ifndef PRIMITIVES_FINALBLOCK_H_
#define PRIMITIVES_FINALBLOCK_H_

#include "ProposedBlock.h"

using namespace Devcash;

namespace Devcash {

class FinalBlock {
 public:
  /**
   * Constructor
   * @param proposed
   */
  explicit FinalBlock(const ProposedBlock& proposed)
      : num_bytes_(proposed.num_bytes_ + 40),
        block_time_(getEpoch()),
        prev_hash_(proposed.prev_hash_),
        merkle_root_(),
        tx_size_(proposed.tx_size_),
        sum_size_(proposed.sum_size_),
        val_count_(proposed.val_count_),
        transaction_vector_(copy(proposed.getTransactions())),
        summary_(proposed.getSummary()),
        vals_(proposed.getValidation()),
        block_state_(proposed.getBlockState()) {
    merkle_root_ = dcHash(getBlockDigest());
    std::vector<byte> merkle(std::begin(merkle_root_), std::end(merkle_root_));
    LOG_INFO << "Merkle: " + toHex(merkle);
  }

  /**
   * Constructor
   * @param serial
   * @param prior
   * @param keys
   * @param tcm
   */
  FinalBlock(const std::vector<byte>& serial,
             const ChainState& prior,
             const KeyRing& keys,
             TransactionCreationManager& tcm)
      : num_bytes_(0),
        block_time_(0),
        prev_hash_(),
        merkle_root_(),
        tx_size_(0),
        sum_size_(0),
        val_count_(0),
        transaction_vector_(),
        summary_(),
        vals_(),
        block_state_(prior) {
    if (serial.size() < minSize()) {
      LOG_WARNING << "Invalid serialized FinalBlock, too small!";
      return;
    }
    version_ |= serial.at(0);
    if (version_ != 0) {
      LOG_WARNING << "Invalid FinalBlock.version: " + std::to_string(version_);
      return;
    }
    size_t offset = 1;
    num_bytes_ = BinToUint64(serial, offset);
    offset += 8;
    if (serial.size() != num_bytes_) {
      LOG_WARNING << "Invalid serialized FinalBlock, wrong size!";
      return;
    }
    block_time_ = BinToUint64(serial, offset);
    offset += 8;
    std::copy_n(serial.begin() + offset, SHA256_DIGEST_LENGTH, prev_hash_.begin());
    offset += 32;
    std::copy_n(serial.begin() + offset, SHA256_DIGEST_LENGTH, merkle_root_.begin());
    offset += 32;
    tx_size_ = BinToUint64(serial, offset);
    offset += 8;
    sum_size_ = BinToUint64(serial, offset);
    offset += 8;
    val_count_ = BinToUint32(serial, offset);
    offset += 4;

    tcm.set_keys(&keys);
    tcm.CreateTransactions(serial, transaction_vector_, offset, minSize(), tx_size_);

    /*
    while (offset < minSize()+tx_size_) {
      //Transaction constructor increments offset by ref
      Transaction one_tx(serial, offset, keys);
      transaction_vector_.push_back(one_tx);
    }
    */

    Summary temp(serial, offset);
    summary_ = temp;
    Validation val_temp(serial, offset);
    vals_ = val_temp;
  }

  /**
   * Constructor
   * @param serial
   * @param prior
   * @param offset
   */
  FinalBlock(const std::vector<byte>& serial, const ChainState& prior, size_t& offset)
      : num_bytes_(0),
        block_time_(0),
        prev_hash_(),
        merkle_root_(),
        tx_size_(0),
        sum_size_(0),
        val_count_(0),
        transaction_vector_(),
        summary_(),
        vals_(),
        block_state_(prior) {
    if (serial.size() < minSize()) {
      LOG_WARNING << "Invalid serialized FinalBlock, too small!";
      return;
    }
    version_ |= serial.at(offset);
    if (version_ != 0) {
      LOG_WARNING << "Invalid FinalBlock.version: " + std::to_string(version_);
      return;
    }
    offset++;
    num_bytes_ = BinToUint64(serial, offset);
    offset += 8;
    if (serial.size() < num_bytes_) {
      LOG_WARNING << "Invalid serialized FinalBlock, wrong size!";
      return;
    }
    block_time_ = BinToUint64(serial, offset);
    offset += 8;
    std::copy_n(serial.begin() + offset, SHA256_DIGEST_LENGTH, prev_hash_.begin());
    offset += 32;
    std::copy_n(serial.begin() + offset, SHA256_DIGEST_LENGTH, merkle_root_.begin());
    offset += 32;
    tx_size_ = BinToUint64(serial, offset);
    offset += 8;
    sum_size_ = BinToUint64(serial, offset);
    offset += 8;
    val_count_ = BinToUint32(serial, offset);
    offset += 4;

    // this constructor does not load transactions
    offset += tx_size_;

    Summary temp(serial, offset);
    summary_ = temp;
    Validation val_temp(serial, offset, val_count_);
    vals_ = val_temp;
  }

  /**
   * Constructor
   * @param other
   */
  FinalBlock(const FinalBlock& other)
      : version_(other.version_)
      , num_bytes_(other.num_bytes_)
      , block_time_(other.block_time_)
      , prev_hash_(other.prev_hash_)
      , merkle_root_(other.merkle_root_)
      , tx_size_(other.tx_size_)
      , sum_size_(other.sum_size_)
      , val_count_(other.val_count_)
      , transaction_vector_(copy(other.transaction_vector_))
      , summary_(other.summary_)
      , vals_(other.vals_)
      , block_state_(other.block_state_){}

  /**
   * Static method which returns the minimum size for a FinalBlock
   * @return minimum size (101)
   */
  static size_t minSize() {
    /// @todo (mckenney) define hard-coded value
    return 101;
  }

  /**
   * Returns the number of transactions in this block
   * @return number of transactions
   */
  size_t getNumTransactions() const { return transaction_vector_.size(); }

  /**
   * Returns copies of the transactions recorded in this block.
   * @return a vector of pointers to transactions
   */
  std::vector<TransactionPtr> getTransactions() const {
    std::vector<TransactionPtr> out;
    for (const auto& e : transaction_vector_) {
      out.push_back(e->clone());
    }
    return out;
  }

  /**
   * Returns the number of coin transfers in this block.
   * @return number of transfers
   */
  size_t getNumTransfers() const {
    size_t tfers = 0;
    for (auto const& item : transaction_vector_) {
      tfers += item->getTransfers().size();
    }
    return tfers;
  }

  /**
   * Returns a JSON representation of this block as a string.
   * @return a JSON representation of this block as a string.
   */
  std::string getJSON() const {
    std::string json("{\"" + kVERSION_TAG + "\":");
    json += std::to_string(version_) + ",";
    json += "\"" + kBYTES_TAG + "\":" + std::to_string(num_bytes_) + ",";
    json += "\"" + kTIME_TAG + "\":" + std::to_string(block_time_) + ",";
    std::vector<byte> prev_hash(std::begin(prev_hash_), std::end(prev_hash_));
    json += "\"" + kPREV_HASH_TAG + "\":" + toHex(prev_hash) + ",";
    std::vector<byte> merkle(std::begin(merkle_root_), std::end(merkle_root_));
    json += "\"" + kMERKLE_TAG + "\":" + toHex(merkle) + ",";
    json += "\"" + kTX_SIZE_TAG + "\":" + std::to_string(tx_size_) + ",";
    json += "\"" + kSUM_SIZE_TAG + "\":" + std::to_string(sum_size_) + ",";
    json += "\"" + kVAL_COUNT_TAG + "\":" + std::to_string(val_count_) + ",";
    json += "\"" + kTXS_TAG + "\":[";
    bool isFirst = true;
    for (auto const& item : transaction_vector_) {
      if (isFirst) {
        isFirst = false;
      } else {
        json += ",";
      }
      json += item->getJSON();
    }
    json += "],\"" + kSUM_TAG + "\":" + summary_.getJSON() + ",";
    json += "\"" + kVAL_TAG + "\":" + vals_.getJSON() + "}";
    return json;
  }

  /**
   * Returns block digest as a byte vector
   * @return
   */
  std::vector<byte> getBlockDigest() const {
    std::vector<byte> txs;
    for (auto const& item : transaction_vector_) {
      const std::vector<byte> txs_canon(item->getCanonical());
      txs.insert(txs.end(), txs_canon.begin(), txs_canon.end());
    }
    const std::vector<byte> sum_canon(summary_.getCanonical());
    const std::vector<byte> val_canon(vals_.getCanonical());

    std::vector<byte> serial;
    serial.reserve(num_bytes_);
    serial.push_back(version_ & 0xFF);
    Uint64ToBin(num_bytes_, serial);
    Uint64ToBin(block_time_, serial);
    serial.insert(serial.end(), prev_hash_.begin(), prev_hash_.end());
    Uint64ToBin(tx_size_, serial);
    Uint64ToBin(sum_size_, serial);
    Uint32ToBin(val_count_, serial);
    serial.insert(serial.end(), txs.begin(), txs.end());
    serial.insert(serial.end(), sum_canon.begin(), sum_canon.end());
    serial.insert(serial.end(), val_canon.begin(), val_canon.end());

    return serial;
  }

  /**
   * Returns a CBOR representation of this block as a byte vector.
   * @return a CBOR representation of this block as a byte vector.
   */
  std::vector<byte> getCanonical() const {
    std::vector<byte> txs;
    for (auto const& item : transaction_vector_) {
      const std::vector<byte> txs_canon(item->getCanonical());
      txs.insert(txs.end(), txs_canon.begin(), txs_canon.end());
    }
    const std::vector<byte> sum_canon(summary_.getCanonical());
    const std::vector<byte> val_canon(vals_.getCanonical());

    std::vector<byte> serial;
    serial.reserve(num_bytes_);
    serial.push_back(version_ & 0xFF);
    Uint64ToBin(num_bytes_, serial);
    Uint64ToBin(block_time_, serial);
    serial.insert(serial.end(), prev_hash_.begin(), prev_hash_.end());
    serial.insert(serial.end(), merkle_root_.begin(), merkle_root_.end());
    Uint64ToBin(tx_size_, serial);
    Uint64ToBin(sum_size_, serial);
    Uint32ToBin(val_count_, serial);
    serial.insert(serial.end(), txs.begin(), txs.end());
    serial.insert(serial.end(), sum_canon.begin(), sum_canon.end());
    serial.insert(serial.end(), val_canon.begin(), val_canon.end());

    return serial;
  }

  /**
   * Return the time of this block
   * @return block time in milliseconds since the Epoch
   */
  uint64_t getBlockTime() const { return block_time_; }

  /**
   *
   * @return
   */
  Hash getMerkleRoot() const { return merkle_root_; }

  /**
   *
   * @return
   */
  ChainState getChainState() const { return block_state_; }

  /**
   *
   * @return
   */
  Summary getSummary() const { return summary_; }

  /**
   *
   * @return
   */
  Validation getValidation() const { return vals_; }

 private:
  /// Version of the block
  uint8_t version_ = 0;
  /// Number of bytes of this block
  uint64_t num_bytes_ = 0;
  /// Number of milliseconds since the Epoch
  uint64_t block_time_ = 0;
  /// The has of the previous block
  Hash prev_hash_;
  /// The merkle root
  Hash merkle_root_;
  /// Size of Transactions in this block
  uint64_t tx_size_ = 0;
  /// Size of the Summary
  uint64_t sum_size_ = 0;
  /// Number of Validations
  uint32_t val_count_ = 0;
  /// vector of TransactionPtrs
  std::vector<TransactionPtr> transaction_vector_;
  /// Summary
  Summary summary_;
  /// Validation
  Validation vals_;
  /// ChainState
  ChainState block_state_;
};

typedef std::shared_ptr<FinalBlock> FinalPtr;

}  // end namespace Devcash

#endif /* PRIMITIVES_FINALBLOCK_H_ */
