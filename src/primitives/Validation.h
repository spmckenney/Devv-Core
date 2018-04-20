/*
 * validation.h defines the structure of the validation section of a block.
 *
 *  Created on: Jan 3, 2018
 *  Author: Nick Williams
 *
 *  **Validation Structure**
 *  The validation block has two parts.
 *  1. A summary of the change in chain state caused by the transactions.
 *  2. A map of miner addresses to signatures.
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
#include <map>
#include <vector>

#include "smartcoin.h"

namespace Devcash
{
typedef std::map<Address, Signature> vmap;

class Validation {
 public:
  vmap sigs_;

/** Constrcutors */
  Validation();
  Validation(std::vector<byte> serial) {}
  Validation(const Validation& other);
  Validation(Address node, Signature sig);
  Validation(vmap sigs);

/** Adds a validation record to this block.
 *  @param node the address of node that produced this validation
 *  @param sig the signature of the validating node
 *  @return true iff the validation verified against the node's public key
*/
  bool addValidation(Address node, Signature sig);
  bool addValidation(Validation& other);

  /** Calculates summary for this block based on provided transactions.
   *  @param a vector of transactions in this block
   *  @return the canonical form of the summary
  */
  //std::string CalculateSummary(const std::vector<DCTransaction>& txs);

/** Returns a JSON string representing this validation block.
 *  @return a JSON string representing this validation block.
*/
  std::string ToJSON() const;

/** Returns a CBOR byte vector representing this validation block.
 *  @return a CBOR byte vector representing this validation block.
*/
  std::vector<byte> ToCanonical() const;

/** Returns the size of this verification block in bytes
 *  @return the size of this verification block in bytes
*/
  unsigned int GetByteSize() const;

/** Returns the number of validations in this block.
 *  @return the number of validations in this block.
*/
  unsigned int GetValidationCount() const;

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
