/*
 * block.h defines the structure of a Devcash block.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 *
 *  **Block Structure**
 *
 *  -Headers-
 *  nVersion
 *  previous blockhash
 *  hashMerkleRoot
 *  blocksize (nBits)
 *  timestamp (nTime)
 *  transaction size (bytes)
 *  summary size (bytes)
 *  validation size (bytes)
 *
 *  -Transactions-
 *  -Summaries-
 *  -Validations-
 */

#ifndef DEVCASH_PRIMITIVES_BLOCK_H
#define DEVCASH_PRIMITIVES_BLOCK_H

#include "consensus/KeyRing.h"
#include "Summary.h"
#include "Transaction.h"
#include "Validation.h"

namespace Devcash
{

class DCBlock {

public:

  const Validation& GetValidationBlock() const {
    return vals_;
  }

  Validation& GetValidationBlock() {
    return vals_;
  }

  std::vector<Devcash::Transaction> vtx_;

  uint32_t vSize_;
  uint32_t sumSize_;
  uint32_t txSize_;
  uint64_t nTime_;
  uint32_t nBytes_;

  short nVersion_ = 0;
  std::string hashPrevBlock_;
  std::string hashMerkleRoot_;

/** Constructor */
  DCBlock();
  DCBlock(const std::string& rawBlock, const KeyRing& keys);
  DCBlock(const DCBlock& other);
  DCBlock(const std::vector<Devcash::Transaction>& txs,
      const Validation& validations);

  DCBlock* operator=(DCBlock&& other)
  {
    if (this != &other) {
      this->vtx_ = std::move(other.vtx_);
      this->nVersion_ = other.nVersion_;
      this->hashPrevBlock_ = other.hashPrevBlock_;
      this->hashMerkleRoot_ = other.hashMerkleRoot_;
      this->nBytes_ = other.nBytes_;
      this->nTime_ = other.nTime_;
      this->txSize_ = other.txSize_;
      this->sumSize_ = other.sumSize_;
      this->vSize_ = other.vSize_;
    }
    return this;
  }

  DCBlock* operator=(const DCBlock& other)
  {
    if (this != &other) {
      this->vtx_ = std::move(other.vtx_);
      this->nVersion_ = other.nVersion_;
      this->hashPrevBlock_ = other.hashPrevBlock_;
      this->hashMerkleRoot_ = other.hashMerkleRoot_;
      this->nBytes_ = other.nBytes_;
      this->nTime_ = other.nTime_;
      this->txSize_ = other.txSize_;
      this->sumSize_ = other.sumSize_;
      this->vSize_ = other.vSize_;
    }
    return this;
  }

  bool compare(const DCBlock& other) const {
    if (hashPrevBlock_ == other.hashPrevBlock_
        && txSize_ == other.txSize_
        && sumSize_ == other.sumSize_) return true;
    return false;
  }

  void copyHeaders(const DCBlock& other) {
    this->nVersion_ = other.nVersion_;
    this->hashPrevBlock_ = other.hashPrevBlock_;
    this->hashMerkleRoot_ = other.hashMerkleRoot_;
    this->nBytes_ = other.nBytes_;
    this->nTime_ = other.nTime_;
    this->txSize_ = other.txSize_;
    this->sumSize_ = other.sumSize_;
    this->vSize_ = other.vSize_;
  }

  DCState& GetBlockState() {
    return block_state_;
  }

  const DCState& GetBlockState() const {
    return block_state_;
  }

  bool SetBlockState(const DCState& prior_state);

  bool addTransaction(std::string txStr, KeyRing& keys);

/** Validates this block.
 *  @pre OpenSSL is initialized and ecKey contains a public key
 *  @note Invalid transactions are removed.
 *  If no valid transactions exist in this block, it is entirely invalid.
 *  @param ecKey the public key to use to validate this block.
 *  @return true iff at least once transaction in this block validated.
 *  @return false if this block has no valid transactions
*/
  bool validate(const KeyRing& keys) const;

/** Signs this block.
 *  @pre OpenSSL is initialized and ecKey contains a private key
 *  @param ecKey the private key to use to sign this block.
 *  @return true iff the block was signed.
 *  @return false otherwise
*/
  bool signBlock(EC_KEY* ecKey, std::string myAddr);

/** Finalizes this block.
 *  Creates the header, calculates the Merkle root, sets the blocktime (nTime_)
 *  @param prevHash a string of the previous block's hash
 *  @return true iff the block was finalized.
 *  @return false otherwise
*/
  bool finalize(const std::string& prevHash);

/** Resets this block. */
  void SetNull()
  {
    nVersion_ = 0;
    hashPrevBlock_ = "";
    hashMerkleRoot_ = "";
    nTime_ = 0;
    nBytes_ = 0;
    txSize_ = 0;
    vSize_ = 0;
  }

/** Returns a JSON representation of this block as a string.
 *  @return a JSON representation of this block as a string.
*/
  std::string ToJSON() const;

/** Returns a CBOR representation of this block as a byte vector.
 *  @return a CBOR representation of this block as a byte vector.
*/
  std::vector<uint8_t> ToCBOR() const;

/** Returns a CBOR representation of this block as a hex string.
 *  @return a CBOR representation of this block as a hex string.
*/
  std::string ToCBOR_str() const;

private:
  Validation vals_;
  DCState block_state_;
  Summary summary_;

};

} //end namespace Devcash

#endif // DEVCASH_PRIMITIVES_BLOCK_H
