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

using namespace Devcash;

namespace Devcash
{

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
    , const KeyRing& keys)
    : num_bytes_(0), prev_hash_(), tx_size_(0), sum_size_(0), val_count_(0)
    , vtx_(), summary_(), vals_(), block_state_(prior) {
    if (serial.size() < MinSize()) {
      LOG_WARNING << "Invalid serialized ProposedBlock, too small!";
      return;
    }
    version_ |= serial.at(0);
    if (version_ != 0) {
      LOG_WARNING << "Invalid ProposedBlock.version: "+std::to_string(version_);
      return;
    }
    size_t offset = 1;
    num_bytes_ = BinToUint64(serial, offset);
    offset += 8;
    if (serial.size() != num_bytes_) {
      LOG_WARNING << "Invalid serialized ProposedBlock, wrong size!";
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
    while (offset < MinSize()+tx_size_) {
      //Transaction constructor increments offset by ref
      Transaction one_tx(serial, offset, keys);
      vtx_.push_back(one_tx);
    }
    Summary temp(serial, offset);
    summary_ = temp;
    Validation val_temp(serial, offset, val_count_);
    vals_ = val_temp;
  }

  ProposedBlock(const ProposedBlock& other)
    : version_(other.version_), num_bytes_(other.num_bytes_)
    , prev_hash_(other.prev_hash_), tx_size_(other.tx_size_)
    , sum_size_(other.sum_size_), val_count_(other.val_count_)
    , vtx_(other.vtx_), summary_(other.summary_), vals_(other.vals_)
    , block_state_(other.block_state_)
  {}

  ProposedBlock(const Hash& prev_hash, const std::vector<Transaction>& txs
      , const Summary& summary, const Validation& validations
      , const ChainState& prior_state) : num_bytes_(0), prev_hash_(prev_hash)
      , tx_size_(0), sum_size_(summary.getByteSize())
      , val_count_(validations.sigs_.size()), vtx_(txs), summary_(summary)
      , vals_(validations), block_state_(prior_state) {

    for (auto const& item : vtx_) {
      tx_size_ += item.getByteSize();
    }

    num_bytes_ = MinSize()+tx_size_+sum_size_
        +val_count_*vals_.PairSize();
  }

  ProposedBlock* operator=(ProposedBlock&& other)
  {
    if (this != &other) {
      this->version_ = other.version_;
      this->num_bytes_ = other.num_bytes_;
      this->prev_hash_ = other.prev_hash_;
      this->tx_size_ = other.tx_size_;
      this->sum_size_ = other.sum_size_;
      this->val_count_ = other.val_count_;
      this->vtx_ = other.vtx_;
      this->summary_ = other.summary_;
      this->vals_ = other.vals_;
      this->block_state_ = other.block_state_;
    }
    return this;
  }

  ProposedBlock* operator=(const ProposedBlock& other)
  {
    if (this != &other) {
      this->version_ = other.version_;
      this->num_bytes_ = other.num_bytes_;
      this->prev_hash_ = other.prev_hash_;
      this->tx_size_ = other.tx_size_;
      this->sum_size_ = other.sum_size_;
      this->val_count_ = other.val_count_;
      this->vtx_ = other.vtx_;
      this->summary_ = other.summary_;
      this->vals_ = other.vals_;
      this->block_state_ = other.block_state_;
    }
    return this;
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
    std::vector<byte> md = summary_.getCanonical();
    size_t node_num = context.get_current_node();
    Address node_addr;
    Signature node_sig;
    std::vector<byte> addr_bin(Hex2Bin(context.kNODE_ADDRs[node_num]));
    std::copy_n(addr_bin.begin(), kADDR_SIZE, node_addr.begin());
    SignBinary(keys.getNodeKey(context.get_current_node()), dcHash(md), node_sig);
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
    if (remote.size() < MinValidationSize()) {
      LOG_WARNING << "Invalid validation data, too small!";
      return false;
    }
    LOG_DEBUG << "ProposedBlock checking validation data.";
    Hash incoming_hash;
    std::copy_n(remote.begin(), SHA256_DIGEST_LENGTH, incoming_hash.begin());
    if (incoming_hash == prev_hash_) { //validations are for this proposal
      size_t offset = SHA256_DIGEST_LENGTH;
      Validation val_temp(remote, offset, val_count_);
      vals_.addValidation(val_temp);
      if (vals_.GetValidationCount() > (context.get_peer_count()/2)) {
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
      json += item.getJSON();
    }
    json += "],\""+kSUM_TAG+"\":"+summary_.getJSON()+",";
    json += "\""+kVAL_TAG+"\":"+vals_.getJSON()+"}";
    return json;
  }

/** Returns a CBOR representation of this block as a byte vector.
 *  @return a CBOR representation of this block as a byte vector.
*/
  std::vector<byte> getCanonical() const {
    std::vector<byte> txs;
    for (auto const& item : vtx_) {
      const std::vector<byte> txs_canon(item.getCanonical());
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

  std::vector<Transaction> getTransactions() const {
    return vtx_;
  }

  Summary getSummary() const {
    return summary_;
  }

  std::vector<byte> getValidationData() {
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

private:
  std::vector<Transaction> vtx_;
  Summary summary_;
  Validation vals_;
  ChainState block_state_;

};

} //end namespace Devcash



#endif /* PRIMITIVES_PROPOSEDBLOCK_H_ */
