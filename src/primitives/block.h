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
 *  validation size (bytes)
 *
 *  -Transactions-
 *  -Validations-
 */

#ifndef DEVCASH_PRIMITIVES_BLOCK_H
#define DEVCASH_PRIMITIVES_BLOCK_H

#include "transaction.h"
#include "validation.h"

namespace Devcash
{

class DCBlockHeader
{
 public:

  short nVersion_ = 0;
  std::string hashPrevBlock_;
  std::string hashMerkleRoot_;
  uint32_t nBytes_;
  uint64_t nTime_;
  uint32_t txSize_;
  uint32_t vSize_;

/** Constructors */
  DCBlockHeader()
  {
      SetNull();
  }

  DCBlockHeader(std::string hashPrevBlock, std::string hashMerkleRoot,
      uint32_t nBytes, uint64_t nTime, uint32_t txSize, uint32_t vSize)
  {
    this->hashPrevBlock_=hashPrevBlock;
    this->hashMerkleRoot_=hashMerkleRoot;
    this->nBytes_=nBytes;
    this->nTime_=nTime;
    this->txSize_=txSize;
    this->vSize_=vSize;
  }

/** Sets this to an empty block. */
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

/** Checks if this is an empty block.
 *  @return true iff this block is empty and invalid
*/
  bool IsNull() const
  {
    return (nBytes_ == 0);
  }

/** Returns the hash of this block.
 *  @return the hash of this block.
*/
  std::string GetHash() const;

/** Returns the time when this block was finalized.
 *  @return the local finalization time of this block, if it is final
 *  @return 0 otherwise
*/
  int64_t GetBlockTime() const
  {
      return (int64_t)nTime_;
  }

/** Returns a JSON representation of this block as a string.
 *  @return a JSON representation of this block as a string.
*/
  std::string ToJSON() const;

/** Returns a CBOR representation of this block as a byte vector.
 *  @return a CBOR representation of this block as a byte vector.
*/
  std::vector<uint8_t> ToCBOR() const;
};


class DCBlock : public DCBlockHeader
{
 public:

  std::vector<Devcash::DCTransaction>& vtx_;
  DCValidationBlock& vals_;

/** Constructor */
  DCBlock(std::vector<Devcash::DCTransaction> &txs,
      DCValidationBlock &validations);

/** Validates this block.
 *  @pre OpenSSL is initialized and ecKey contains a public key
 *  @note Invalid transactions are removed.
 *  If no valid transactions exist in this block, it is entirely invalid.
 *  @param ecKey the public key to use to validate this block.
 *  @return true iff at least once transaction in this block validated.
 *  @return false if this block has no valid transactions
*/
  bool validate(EC_KEY* ecKey);

/** Signs this block.
 *  @pre OpenSSL is initialized and ecKey contains a private key
 *  @param ecKey the private key to use to sign this block.
 *  @return true iff the block was signed.
 *  @return false otherwise
*/
  bool signBlock(EC_KEY* ecKey);

/** Finalizes this block.
 *  Creates the header, calculates the Merkle root, sets the blocktime (nTime_)
 *  @return true iff the block was finalized.
 *  @return false otherwise
*/
  bool finalize();

/** Resets this block. */
  void SetNull()
  {
      DCBlockHeader::SetNull();
  }

/** Get the header for this block.
 *  @return the DCBlockHeader for this block
*/
  DCBlockHeader GetBlockHeader() const
  {
      DCBlockHeader block;
      block.nVersion_       = nVersion_;
      block.hashPrevBlock_  = hashPrevBlock_;
      block.hashMerkleRoot_ = hashMerkleRoot_;
      block.nTime_          = nTime_;
      block.nBytes_          = nBytes_;
      return block;
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
  std::string ToCBOR_str();
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct DCBlockLocator
{
  std::vector<std::string> vHave_;

/** Constructors */
  DCBlockLocator() {}

  explicit DCBlockLocator(const std::vector<std::string>& vHaveIn) : vHave_(vHaveIn) {}

  /** Resets this locator. */
  void SetNull()
  {
      vHave_.clear();
  }

/** Checks if this is an empty locator.
 *  @return true iff this locator is empty.
*/
  bool IsNull() const
  {
      return vHave_.empty();
  }
};

} //end namespace Devcash

#endif // DEVCASH_PRIMITIVES_BLOCK_H

