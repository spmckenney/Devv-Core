/*
 * validation.h defines the structure of the validation section of a block.
 *
 *  Created on: Jan 3, 2018
 *  Author: Nick Williams
 *
 *  **Validation Structure**
 *  A map of miner addresses to signatures.
 *
 *  **Core Validation Logic**
 *  All of the following statements must be true;
 *  1. The sum of all xfer.amounts in a transaction must be 0.
 *  2. Every xfer.sig must validate against its xfer.addr.
 *  3. In 'exchange' operations, for each sender, xfer.amount must exactly equal the current sum of its onchain coins.
 *  4. In 'create','modify','delete' operations, each sender addr must be part of the INN.
 *
 */

#ifndef SRC_PRIMITIVES_VALIDATION_H_
#define SRC_PRIMITIVES_VALIDATION_H_

#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace Devcash
{

typedef std::unordered_map<std::string, std::string> vmap;

class DCValidationBlock {
 public:
  vmap sigs_;

/** Checks if this is an empty validation block.
 *  @return true iff this validation block is empty and invalid
*/
  bool IsNull() const { return (sigs_.size() < 1); }

/** Constrcutors */
  DCValidationBlock();
  DCValidationBlock(std::vector<uint8_t> cbor);
  DCValidationBlock(std::string jsonMsg);
  DCValidationBlock(const DCValidationBlock& other);
  DCValidationBlock(std::string node, std::string sig);
  DCValidationBlock(std::unordered_map<std::string, std::string> sigs);

/** Adds a validation record to this block.
 *  @param node a string containing the URI of node that produced this validation
 *  @param sig the signature of the validating node
 *  @return true iff the validation verified against the node's public key
*/
  bool addValidation(std::string node, std::string sig);

/** Returns a JSON string representing this validation block.
 *  @return a JSON string representing this validation block.
*/
  std::string ToJSON() const;

/** Returns a CBOR byte vector representing this validation block.
 *  @return a CBOR byte vector representing this validation block.
*/
  std::vector<uint8_t> ToCBOR() const;

/** Returns the size of this verification block in bytes
 *  @return the size of this verification block in bytes
*/
  unsigned int GetByteSize() const;

/** Returns the number of validations in this block.
 *  @return the number of validations in this block.
*/
  unsigned int GetValidationCount() const;

/** Comparison Operators */
  friend bool operator==(const DCValidationBlock& a, const DCValidationBlock& b)
  {
      return a.hash_ == b.hash_;
  }

  friend bool operator!=(const DCValidationBlock& a, const DCValidationBlock& b)
  {
      return a.hash_ != b.hash_;
  }

/** Assignment Operators */
  DCValidationBlock* operator=(DCValidationBlock& other)
  {
    if (this != &other) {
    this->hash_ = other.hash_;
    this->jsonStr_ = other.jsonStr_;
    this->sigs_ = std::move(other.sigs_);
    }
    return this;
  }

/** Returns the hash of this validation block.
 *  @return the hash of this validation block.
*/
  const std::string& GetHash() const {
      return hash_;
  }

 private:
  std::string hash_ = "";
  std::string jsonStr_ = "";

/** Recalculates and returns the hash of this validation block.
 *  @return the hash of this validation block.
*/
  std::string ComputeHash() const;

};

} //end namespace Devcash

#endif /* SRC_PRIMITIVES_VALIDATION_H_ */
