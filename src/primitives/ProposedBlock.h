/*
 * ProposedBlock.h
 *
 *  Created on: Apr 20, 2018
 *      Author: Nick Williams
 */

#ifndef PRIMITIVES_PROPOSEDBLOCK_H_
#define PRIMITIVES_PROPOSEDBLOCK_H_

#include "Summary.h"
#include "Transaction.h"
#include "Validation.h"
#include "concurrency/TransactionCreationManager.h"

using namespace Devcash;

namespace Devcash
{

static std::vector<TransactionPtr> copy(const std::vector<TransactionPtr>& txs) {
  std::vector<TransactionPtr> tx_out;
  for (auto& iter : txs) {
    tx_out.push_back(iter->Clone());
  }
  return(std::move(tx_out));
}

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

class ProposedBlock {

public:

  uint8_t version_ = 0;
  uint64_t num_bytes_;
  Hash prev_hash_;
  uint64_t tx_size_;
  uint64_t sum_size_;
  uint32_t val_count_;

  /** Constructors */
  ProposedBlock(const ChainState& prior) : num_bytes_(0), prev_hash_()
  , tx_size_(0), sum_size_(0), val_count_(0), vtx_(), summary_(), vals_()
  , block_state_(prior)
  {}

  ProposedBlock(const std::vector<byte>& serial, const ChainState& prior
                , const KeyRing& keys, TransactionCreationManager& tcm)
    : num_bytes_(0), prev_hash_(), tx_size_(0), sum_size_(0), val_count_(0)
    , vtx_(), summary_(), vals_(), block_state_(prior) {
    MTR_SCOPE_FUNC();

    int proposed_block_int = 123;
    MTR_START("proposed_block", "proposed_block", &proposed_block_int);

    if (serial.size() < MinSize()) {
      LOG_WARNING << "Invalid serialized ProposedBlock, too small!";
      MTR_FINISH("proposed_block", "construct", &proposed_block_int);
      return;
    }
    version_ |= serial.at(0);
    if (version_ != 0) {
      LOG_WARNING << "Invalid ProposedBlock.version: "+std::to_string(version_);
      MTR_FINISH("proposed_block", "construct", &proposed_block_int);
      return;
    }
    size_t offset = 1;
    num_bytes_ = BinToUint64(serial, offset);
    offset += 8;
    MTR_STEP("proposed_block", "construct", &proposed_block_int, "step1");
    if (serial.size() != num_bytes_) {
      LOG_WARNING << "Invalid serialized ProposedBlock, wrong size!";
      MTR_FINISH("proposed_block", "construct", &proposed_block_int);
      return;
    }
    std::copy_n(serial.begin()+offset, SHA256_DIGEST_LENGTH, prev_hash_.begin());
    offset += 32;
    tx_size_ = BinToUint64(serial, offset);
    offset += 8;
    sum_size_ = BinToUint64(serial, offset);
    offset += 8;
    val_count_ = BinToUint32(serial, offset);
    offset += 4;
    MTR_STEP("proposed_block", "construct", &proposed_block_int, "transaction list");
    tcm.set_keys(&keys);
    tcm.CreateTransactions(serial,
                            vtx_,
                            offset,
                            MinSize(),
                            tx_size_);
    /*

                          while (offset < MinSize()+tx_size_) {
      //Transaction constructor increments offset by ref
      LOG_DEBUG << "while, offset = " << offset;
      Transaction one_tx(serial, offset, keys, false);
      vtx_.push_back(one_tx);
    }
    MTR_STEP("proposed_block", "construct", &proposed_block_int, "soundness");

    boost::asio::io_service io_service;
    boost::thread_group threads;
    boost::asio::io_service::work work(io_service);

    for (unsigned int i = 0; i < boost::thread::hardware_concurrency(); ++i)
    {
        threads.create_thread(boost::bind(&boost::asio::io_service::run,
                                          &io_service));
    }
    std::vector<boost::shared_future<bool>> pending_data; // vector of futures
    // Submit a lambda object to the pool.
    for (auto& tx : vtx_) {
      push_job(tx, keys, io_service, pending_data);
      //tx.setIsSound();
    }

    boost::wait_for_all(pending_data.begin(), pending_data.end());
    */

    MTR_STEP("proposed_block", "construct", &proposed_block_int, "step3");
    Summary temp(serial, offset);
    summary_ = temp;
    MTR_STEP("proposed_block", "construct", &proposed_block_int, "step4");
    Validation val_temp(serial, offset);
    vals_ = val_temp;
    MTR_FINISH("proposed_block", "construct", &proposed_block_int);
  }

  ProposedBlock(ProposedBlock& other)
    : version_(other.version_), num_bytes_(other.num_bytes_)
    , prev_hash_(other.prev_hash_), tx_size_(other.tx_size_)
    , sum_size_(other.sum_size_), val_count_(other.val_count_)
    , vtx_(std::move(other.vtx_)), summary_(other.summary_), vals_(other.vals_)
    , block_state_(other.block_state_)
  {}

  /*
  ProposedBlock(const ProposedBlock& other)
    : version_(other.version_), num_bytes_(other.num_bytes_)
    , prev_hash_(other.prev_hash_), tx_size_(other.tx_size_)
    , sum_size_(other.sum_size_), val_count_(other.val_count_)
    , vtx_(copy(other.vtx_)), summary_(other.summary_), vals_(other.vals_)
    , block_state_(other.block_state_)
  {}
  */

  ProposedBlock(const Hash& prev_hash, std::vector<TransactionPtr>& txs
                , const Summary& summary, const Validation& validations
                , const ChainState& prior_state)
    : num_bytes_(0), prev_hash_(prev_hash)
    , tx_size_(0), sum_size_(summary.getByteSize())
    , val_count_(validations.sigs_.size()), vtx_(std::move(txs)), summary_(summary)
    , vals_(validations), block_state_(prior_state) {

    MTR_SCOPE_FUNC();
    for (auto const& item : vtx_) {
      tx_size_ += item->getByteSize();
    }

    num_bytes_ = MinSize()+tx_size_+sum_size_
      +val_count_*vals_.PairSize();
  }

  bool isNull() {
    return(num_bytes_ < MinSize());
  }

  void setNull() {
    num_bytes_ = 0;
  }

  Hash getPrevHash() {
    return prev_hash_;
  }

  void setPrevHash(const Hash& prev_hash) {
    prev_hash_ = prev_hash;
  }

  static size_t MinSize() {
    return 61;
  }

  static size_t MinValidationSize() {
    return SHA256_DIGEST_LENGTH+(Validation::PairSize()*2);
  }

/** Validates this block.
 *  @pre OpenSSL is initialized and ecKey contains a public key
 *  @note Invalid transactions are removed.
 *  If no valid transactions exist in this block, it is entirely invalid.
 *  @param ecKey the public key to use to validate this block.
 *  @return true iff at least once transaction in this block validated.
 *  @return false if this block has no valid transactions
*/
  bool validate(const KeyRing& keys) const {
    LOG_DEBUG << "validate()";
    MTR_SCOPE_FUNC();
    if (vtx_.size() < 1) {
      LOG_WARNING << "Trying to validate empty block.";
      return false;
    }

    if (!summary_.isSane()) {
      LOG_WARNING << "Summary is invalid in block.validate()!\n";
      LOG_DEBUG << "Summary state: "+summary_.getJSON();
      return false;
    }

    std::vector<byte> md = summary_.getCanonical();
    for (auto iter = vals_.sigs_.begin(); iter != vals_.sigs_.end(); ++iter) {
      if(!VerifyByteSig(keys.getKey(iter->first), dcHash(md), iter->second)) {
        LOG_WARNING << "Invalid block signature";
        LOG_DEBUG << "Block state: "+getJSON();
        LOG_DEBUG << "Block Node Addr: "+toHex(std::vector<byte>(
            std::begin(iter->first), std::end(iter->first)));
        LOG_DEBUG << "Block Node Sig: "+toHex(std::vector<byte>(
            std::begin(iter->second), std::end(iter->second)));
        return false;
      }
    }

    return true;
  }

/** Signs this block.
 *  @pre OpenSSL is initialized and ecKey contains a private key
 *  @param ecKey the private key to use to sign this block.
 *  @param node_addr the address to use to sign this block.
 *  @return true iff the block was signed.
 *  @return false otherwise
*/
  bool SignBlock(const KeyRing& keys, const DevcashContext& context) {
    MTR_SCOPE_FUNC();
    std::vector<byte> md = summary_.getCanonical();
    size_t node_num = context.get_current_node();
    Address node_addr = keys.getNodeAddr(node_num);
    Signature node_sig;
    SignBinary(keys.getNodeKey(node_num), dcHash(md), node_sig);
    vals_.addValidation(node_addr, node_sig);
    val_count_++;
    num_bytes_ = MinSize()+tx_size_+sum_size_
        +(val_count_*Validation::PairSize());
    return true;
  }

  /** Checks if a remote validation applies to this Proposal
   *  @note Adds the validation to this as a side-effect.
   *  @param remote the validation data from the remote peer
   *  @return true iff this block has enough validations to finalize
   *  @return false otherwise
  */
  bool CheckValidationData(std::vector<byte> remote
      , const DevcashContext& context) {
    MTR_SCOPE_FUNC();
    if (remote.size() < MinValidationSize()) {
      LOG_WARNING << "Invalid validation data, too small!";
      return false;
    }
    LOG_DEBUG << "ProposedBlock checking validation data.";
    Hash incoming_hash;
    std::copy_n(remote.begin(), SHA256_DIGEST_LENGTH, incoming_hash.begin());
    if (incoming_hash == prev_hash_) { //validations are for this proposal
      size_t offset = SHA256_DIGEST_LENGTH;
      Validation val_temp(remote, offset);
      vals_.addValidation(val_temp);
      val_count_ = vals_.GetValidationCount();
      num_bytes_ = MinSize()+tx_size_+sum_size_
        +val_count_*vals_.PairSize();
      if (val_count_ > (context.get_peer_count()/2)) {
        return true;
      }
    } else {
      LOG_WARNING << "Invalid validation data, hash does not match this proposal!";
    }
    return false;
  }

/** Returns a JSON representation of this block as a string.
 *  @return a JSON representation of this block as a string.
*/
  std::string getJSON() const {
    MTR_SCOPE_FUNC();
    std::string json("{\""+kVERSION_TAG+"\":");
    json += std::to_string(version_)+",";
    json += "\""+kBYTES_TAG+"\":"+std::to_string(num_bytes_)+",";
    std::vector<byte> prev_hash(std::begin(prev_hash_), std::end(prev_hash_));
    json += "\""+kPREV_HASH_TAG+"\":"+toHex(prev_hash)+",";
    json += "\""+kTX_SIZE_TAG+"\":"+std::to_string(tx_size_)+",";
    json += "\""+kSUM_SIZE_TAG+"\":"+std::to_string(sum_size_)+",";
    json += "\""+kVAL_COUNT_TAG+"\":"+std::to_string(val_count_)+",";
    json += "\""+kTXS_TAG+"\":[";
    bool isFirst = true;
    for (auto const& item : vtx_) {
      if (isFirst) {
        isFirst = false;
      } else {
        json += ",";
      }
      json += item->getJSON();
    }
    json += "],\""+kSUM_TAG+"\":"+summary_.getJSON()+",";
    json += "\""+kVAL_TAG+"\":"+vals_.getJSON()+"}";
    return json;
  }

/** Returns a CBOR representation of this block as a byte vector.
 *  @return a CBOR representation of this block as a byte vector.
*/
  std::vector<byte> getCanonical() const {
    MTR_SCOPE_FUNC();
    std::vector<byte> txs;
    for (auto const& item : vtx_) {
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

  const std::vector<TransactionPtr>& getTransactions() const {
    return vtx_;
  }

  size_t getNumTransactions() const {
    return vtx_.size();
  }

  Summary getSummary() const {
    return summary_;
  }

  std::vector<byte> getValidationData() {
    MTR_SCOPE_FUNC();
    std::vector<byte> out;
    out.insert(out.end(), prev_hash_.begin(), prev_hash_.end());
    const std::vector<byte> val_canon(vals_.getCanonical());
    out.insert(out.end(), val_canon.begin(), val_canon.end());
    return out;
  }

  Validation getValidation() const {
    return vals_;
  }

  ChainState getBlockState() const {
    return block_state_;
  }

  ProposedBlock& shallowCopy(ProposedBlock& other) {
    version_ = other.version_;
    num_bytes_ = other.num_bytes_;
    prev_hash_ = other.prev_hash_;
    tx_size_ = other.tx_size_;
    sum_size_ = other.sum_size_;
    val_count_ = other.val_count_;
    vtx_ = std::move(other.vtx_);
    summary_ = other.summary_;
    vals_ = other.vals_;
    block_state_ = other.block_state_;
  }

private:
  ProposedBlock& operator=(const ProposedBlock& other);

  std::vector<TransactionPtr> vtx_;
  Summary summary_;
  Validation vals_;
  ChainState block_state_;
};

} //end namespace Devcash



#endif /* PRIMITIVES_PROPOSEDBLOCK_H_ */
