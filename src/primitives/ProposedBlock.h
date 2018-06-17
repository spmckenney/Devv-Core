/*
 * ProposedBlock.h
 *
 *  Created on: Apr 20, 2018
 *      Author: Nick Williams
 */

#ifndef PRIMITIVES_PROPOSEDBLOCK_H_
#define PRIMITIVES_PROPOSEDBLOCK_H_

#include "primitives/block.h"
#include "primitives/buffers.h"
#include "primitives/Summary.h"
#include "primitives/Transaction.h"
#include "primitives/Validation.h"
#include "concurrency/TransactionCreationManager.h"

using namespace Devcash;

namespace Devcash {

/**
 * Perform a deep copy of the vector of Transaction unique_ptrs
 * @param txs
 * @return a copy of the transactions
 */
static std::vector<TransactionPtr> Copy(const std::vector<TransactionPtr> &txs) {
  std::vector<TransactionPtr> tx_out;
  for (auto& iter : txs) {
    tx_out.push_back(iter->clone());
  }
  return (std::move(tx_out));
}

/// @todo (mckenney) move to constants file
static const std::string kVERSION_TAG = "v";
static const std::string kPREV_HASH_TAG = "prev";
static const std::string kMERKLE_TAG = "merkle";
static const std::string kBYTES_TAG = "bytes";
static const std::string kTIME_TAG = "time";
static const std::string kTX_SIZE_TAG = "txlen";
static const std::string kSUM_SIZE_TAG = "sumlen";
static const std::string kVAL_COUNT_TAG = "vcount";
static const std::string kTXS_TAG = "txs";
static const std::string kSUM_TAG = "sum";
static const std::string kVAL_TAG = "vals";

/**
 * A proposed block.
 */
class ProposedBlock {
 public:

  /**
   *
   * @param prior
   */
  explicit ProposedBlock(const ChainState& prior)
      : num_bytes_(0)
      , prev_hash_()
      , tx_size_(0)
      , sum_size_(0)
      , val_count_(0)
      , transaction_vector_()
      , summary_(Summary::Create())
      , vals_()
      , block_state_(prior) {}

  /**
   *
   * @param serial
   * @param prior
   * @param keys
   * @param tcm
   */
  ProposedBlock(InputBuffer& buffer,
                const ChainState& prior,
                const KeyRing& keys,
                TransactionCreationManager& tcm)
      : num_bytes_(0)
      , prev_hash_()
      , tx_size_(0)
      , sum_size_(0)
      , val_count_(0)
      , transaction_vector_()
      , summary_(Summary::Create())
      , vals_()
      , block_state_(prior) {
    MTR_SCOPE_FUNC();

    int proposed_block_int = 123;
    MTR_START("proposed_block", "proposed_block", &proposed_block_int);

    if (buffer.size() < MinSize()) {
      LOG_WARNING << "Invalid serialized ProposedBlock, too small!";
      MTR_FINISH("proposed_block", "construct", &proposed_block_int);
      return;
    }
    version_ |= buffer.getNextByte();
    if (version_ != 0) {
      LOG_WARNING << "Invalid ProposedBlock.version: " + std::to_string(version_);
      MTR_FINISH("proposed_block", "construct", &proposed_block_int);
      return;
    }
    num_bytes_ = buffer.getNextUint64();
    MTR_STEP("proposed_block", "construct", &proposed_block_int, "step1");
    if (buffer.size() != num_bytes_) {
      LOG_WARNING << "Invalid serialized ProposedBlock, wrong size!";
      MTR_FINISH("proposed_block", "construct", &proposed_block_int);
      return;
    }

    buffer.copy(prev_hash_);
    tx_size_ = buffer.getNextUint64();
    sum_size_ = buffer.getNextUint64();
    val_count_ = buffer.getNextUint32();
    MTR_STEP("proposed_block", "construct", &proposed_block_int, "transaction list");
    tcm.set_keys(&keys);
    tcm.CreateTransactions(buffer, transaction_vector_, MinSize(), tx_size_);

    MTR_STEP("proposed_block", "construct", &proposed_block_int, "step3");
    summary_ = Summary::Create(buffer);

    MTR_STEP("proposed_block", "construct", &proposed_block_int, "step4");
    Validation val_temp(buffer);
    vals_ = val_temp;
    MTR_FINISH("proposed_block", "construct", &proposed_block_int);
  }

  static ProposedBlock Create(InputBuffer& buffer,
                const ChainState& prior,
                const KeyRing& keys,
                TransactionCreationManager& tcm);
  /**
   *
   * @param other
   */
  ProposedBlock(ProposedBlock& other)
      : version_(other.version_)
      , num_bytes_(other.num_bytes_)
      , prev_hash_(other.prev_hash_)
      , tx_size_(other.tx_size_)
      , sum_size_(other.sum_size_)
      , val_count_(other.val_count_)
      , transaction_vector_(std::move(other.transaction_vector_))
      , summary_(Summary::Copy(other.summary_))
      , vals_(other.vals_)
      , block_state_(other.block_state_) {}

  /**
   *
   * @param prev_hash
   * @param txs
   * @param summary
   * @param validations
   * @param prior_state
   */
  ProposedBlock(const Hash& prev_hash,
                std::vector<TransactionPtr>& txs,
                const Summary& summary,
                const Validation& validations,
                const ChainState& prior_state)
      : num_bytes_(0)
      , prev_hash_(prev_hash)
      , tx_size_(0)
      , sum_size_(summary.getByteSize())
      , val_count_(validations.getValidationCount())
      , transaction_vector_(std::move(txs))
      , summary_(Summary::Copy(summary))
      , vals_(validations)
      , block_state_(prior_state) {
    MTR_SCOPE_FUNC();
    for (auto const& item : transaction_vector_) {
      tx_size_ += item->getByteSize();
    }

    num_bytes_ = MinSize() + tx_size_ + sum_size_ + val_count_ * vals_.pairSize();
  }

  ProposedBlock& operator=(const ProposedBlock& other) = delete;

  /**
   *
   * @return
   */
  bool isNull() const { return (num_bytes_ < MinSize()); }

  /**
   *
   */
  void setNull() { num_bytes_ = 0; }

  /**
   * Return a const ref to previous hash
   * @return
   */
  const Hash& getPrevHash() const { return prev_hash_; }

  /**
   *
   * @param prev_hash
   */
  void setPrevHash(const Hash& prev_hash) { prev_hash_ = prev_hash; }

  /**
   *
   * @return
   */
  static size_t MinSize() { return 61; }

  /**
   *
   * @return
   */
  static size_t MinValidationSize() { return SHA256_DIGEST_LENGTH + (Validation::pairSize() * 2); }

  /**
   * Validates this block.
   * @pre OpenSSL is initialized and ecKey contains a public key
   * @note Invalid transactions are removed.
   * If no valid transactions exist in this block, it is entirely invalid.
   * @param ecKey the public key to use to validate this block.
   * @return true iff at least once transaction in this block validated.
   * @return false if this block has no valid transactions
   */
  bool validate(const KeyRing& keys) const {
    LOG_DEBUG << "validate()";
    MTR_SCOPE_FUNC();
    if (transaction_vector_.size() < 1) {
      LOG_WARNING << "Trying to validate empty block.";
      return false;
    }

    if (!summary_.isSane()) {
      LOG_WARNING << "Summary is invalid in block.validate()!\n";
      LOG_DEBUG << "Summary state: " + summary_.getJSON();
      return false;
    }

    std::vector<byte> md = summary_.getCanonical();
    for (auto& sig : vals_.getValidationMap()) {
      if (!VerifyByteSig(keys.getKey(sig.first), DevcashHash(md), sig.second)) {
        LOG_WARNING << "Invalid block signature";
        LOG_DEBUG << "Block state: " + getJSON();
        LOG_DEBUG << "Block Node Addr: " + ToHex(std::vector<byte>(std::begin(sig.first), std::end(sig.first)));
        LOG_DEBUG << "Block Node Sig: " + ToHex(std::vector<byte>(std::begin(sig.second), std::end(sig.second)));
        return false;
      }
    }

    return true;
  }

  /**
   * Signs this block.
   * @pre OpenSSL is initialized and ecKey contains a private key
   * @param ecKey the private key to use to sign this block.
   * @param node_addr the address to use to sign this block.
   * @return true iff the block was signed.
   * @return false otherwise
   */
  bool signBlock(const KeyRing &keys, const DevcashContext& context) {
    MTR_SCOPE_FUNC();
    std::vector<byte> md = summary_.getCanonical();
    /// @todo (mckenney) need another solution for node_num with dynamic shards
    size_t node_num = context.get_current_node() % context.get_peer_count();
    Address node_addr = keys.getNodeAddr(node_num);
    Signature node_sig;
    SignBinary(keys.getNodeKey(node_num), DevcashHash(md), node_sig);
    vals_.addValidation(node_addr, node_sig);
    val_count_++;
    num_bytes_ = MinSize() + tx_size_ + sum_size_ + (val_count_ * Validation::pairSize());
    return true;
  }

  /**
   * Checks if a remote validation applies to this Proposal
   * @note Adds the validation to this as a side-effect.
   * @param remote the validation data from the remote peer
   * @return true iff this block has enough validations to finalize
   * @return false otherwise
   */
  bool checkValidationData(InputBuffer& buffer, const DevcashContext& context) {
    MTR_SCOPE_FUNC();
    if (buffer.size() < MinValidationSize()) {
      LOG_WARNING << "Invalid validation data, too small!";
      return false;
    }
    LOG_DEBUG << "ProposedBlock checking validation data.";
    Hash incoming_hash;
    buffer.copy(incoming_hash);
    if (incoming_hash == prev_hash_) {  // validations are for this proposal
      Validation val_temp(buffer);
      vals_.addValidation(val_temp);
      val_count_ = vals_.getValidationCount();
      num_bytes_ = MinSize() + tx_size_ + sum_size_ + val_count_ * vals_.pairSize();
      if (val_count_ > (context.get_peer_count() / 2)) {
        return true;
      }
    } else {
      LOG_WARNING << "Invalid validation data, hash does not match this proposal!";
    }
    return false;
  }

  /**
   * Returns a JSON representation of this block as a string.
   * @return a JSON representation of this block as a string.
   */
  std::string getJSON() const {
    MTR_SCOPE_FUNC();
    std::string json("{\"" + kVERSION_TAG + "\":");
    json += std::to_string(version_) + ",";
    json += "\"" + kBYTES_TAG + "\":" + std::to_string(num_bytes_) + ",";
    std::vector<byte> prev_hash(std::begin(prev_hash_), std::end(prev_hash_));
    json += "\"" + kPREV_HASH_TAG + "\":" + ToHex(prev_hash) + ",";
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
   * Returns a CBOR representation of this block as a byte vector.
   * @return a CBOR representation of this block as a byte vector.
   */
  std::vector<byte> getCanonical() const {
    MTR_SCOPE_FUNC();
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
   *
   * @return
   */
  const std::vector<TransactionPtr>& getTransactions() const { return transaction_vector_; }

  /**
   *
   * @return
   */
  size_t getNumTransactions() const { return transaction_vector_.size(); }

  const Summary& getSummary() const { return summary_; }

  /**
   *
   * @return
   */
  std::vector<byte> getValidationData() {
    MTR_SCOPE_FUNC();
    std::vector<byte> out;
    out.insert(out.end(), prev_hash_.begin(), prev_hash_.end());
    const std::vector<byte> val_canon(vals_.getCanonical());
    out.insert(out.end(), val_canon.begin(), val_canon.end());
    return out;
  }

  /**
   *
   * @return
   */
  Validation getValidation() const { return vals_; }

  /**
   *
   * @return
   */
  const ChainState& getBlockState() const { return block_state_; }

  /**
   * Performs a shallow copy. Moves the transactions from the other ProposedBlock
   * to this block.
   * @param other the ProposedBlock from which to move transactions from
   * @return A reference to this block
   */
  ProposedBlock& shallowCopy(ProposedBlock& other) {
    version_ = other.version_;
    num_bytes_ = other.num_bytes_;
    prev_hash_ = other.prev_hash_;
    tx_size_ = other.tx_size_;
    sum_size_ = other.sum_size_;
    val_count_ = other.val_count_;
    transaction_vector_ = std::move(other.transaction_vector_);
    summary_ = std::move(other.summary_);
    vals_ = other.vals_;
    block_state_ = other.block_state_;
    return *this;
  }

  /**
   * Returns the size (number of bytes)
   * @return Number of bytes
   */
  uint64_t getNumBytes() const { return num_bytes_; }

  /**
   * Returns the size of the transactions
   * @return the size of the transactions
   */
  uint64_t getSizeofTransactions() const { return tx_size_; }

  /**
   * Returns the size of the Summary
   * @return the size of the Summary
   */
  uint64_t getSummarySize() const { return sum_size_; }

  /**
   * Returns the number of validations
   * @return the number of validations
   */
  uint32_t getNumValidations() const { return val_count_; }

 private:

  /**
   * Private default constructor.
   */
  ProposedBlock() = default;

  /// Version of the block
  uint8_t version_ = 0;
  /// Number of bytes in the block
  uint64_t num_bytes_ = 0;
  /// Hash of previous block
  Hash prev_hash_;
  /// Size of Transactions in this block
  uint64_t tx_size_ = 0;
  /// Size of the Summary
  uint64_t sum_size_ = 0;
  /// Number of Validations
  uint32_t val_count_ = 0;
  /// vector of TransactionPtrs
  std::vector<TransactionPtr> transaction_vector_;
  /// Summary
  Summary summary_ = Summary::Create();
  /// Validation
  Validation vals_;
  /// ChainState
  ChainState block_state_;
};

/**
 * Create a new Proposed block from the InputBuffer
 *
 * @param buffer
 * @param prior
 * @param keys
 * @param tcm
 * @return
 */
inline ProposedBlock ProposedBlock::Create(InputBuffer &buffer,
                            const ChainState &prior,
                            const KeyRing &keys,
                            TransactionCreationManager &tcm) {
  MTR_SCOPE_FUNC();
  ProposedBlock new_block;

  int proposed_block_int = 123;
  MTR_START("proposed_block", "proposed_block", &proposed_block_int);

  if (buffer.size() < MinSize()) {
    MTR_FINISH("proposed_block", "construct", &proposed_block_int);
    std::string warning = "Invalid serialized ProposedBlock, too small!";
    LOG_WARNING << warning;
    throw std::runtime_error(warning);
  }
  new_block.version_ |= buffer.getNextByte();
  if (new_block.version_ != 0) {
    MTR_FINISH("proposed_block", "construct", &proposed_block_int);
    std::string warning = "Invalid ProposedBlock.version: " + std::to_string(new_block.version_);
    LOG_WARNING << warning;
    throw std::runtime_error(warning);
  }
  new_block.num_bytes_ = buffer.getNextUint64();
  MTR_STEP("proposed_block", "construct", &proposed_block_int, "step1");
  if (buffer.size() != new_block.num_bytes_) {
    MTR_FINISH("proposed_block", "construct", &proposed_block_int);
    std::string warning = "Invalid serialized ProposedBlock, wrong size!";
    LOG_WARNING << warning;
    throw std::runtime_error(warning);
  }

  buffer.copy(new_block.prev_hash_);
  new_block.tx_size_ = buffer.getNextUint64();
  new_block.sum_size_ = buffer.getNextUint64();
  new_block.val_count_ = buffer.getNextUint32();
  MTR_STEP("proposed_block", "construct", &proposed_block_int, "transaction list");
  tcm.set_keys(&keys);
  tcm.CreateTransactions(buffer, new_block.transaction_vector_, MinSize(), new_block.tx_size_);

  MTR_STEP("proposed_block", "construct", &proposed_block_int, "step3");
  new_block.summary_ = Summary::Create(buffer);

  MTR_STEP("proposed_block", "construct", &proposed_block_int, "step4");
  Validation val_temp(buffer);
  new_block.vals_ = val_temp;
  MTR_FINISH("proposed_block", "construct", &proposed_block_int);

  return new_block;
}

}  // end namespace Devcash

#endif /* PRIMITIVES_PROPOSEDBLOCK_H_ */
