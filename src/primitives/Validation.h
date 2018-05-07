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
  Validation() : sigs_() {}
  Validation(const std::vector<byte>& serial, size_t& offset) : sigs_() {
    MTR_SCOPE_FUNC();
    size_t remainder = serial.size()-offset;
    while (remainder >= PairSize()) {
      Address one_addr;
      Signature one_sig;
      std::copy_n(serial.begin()+offset, kADDR_SIZE, one_addr.begin());
      offset += kADDR_SIZE;
      std::copy_n(serial.begin()+offset, kSIG_SIZE, one_sig.begin());
      offset += kSIG_SIZE;
      std::pair<Address, Signature> one_pair(one_addr, one_sig);
      sigs_.insert(one_pair);
      remainder -= PairSize();
    }
  }
  Validation(const Validation& other) : sigs_(other.sigs_) {}
  Validation(Address node, Signature sig) : sigs_() {
    sigs_.insert(std::pair<Address, Signature>(node, sig));
  }
  Validation(vmap sigs) : sigs_(sigs) {}

/** Adds a validation record to this block.
 *  @param node the address of node that produced this validation
 *  @param sig the signature of the validating node
 *  @return true iff the validation verified against the node's public key
*/
  bool addValidation(Address node, Signature sig) {
    sigs_.insert(std::pair<Address, Signature>(node, sig));
    return true;
  }
  bool addValidation(Validation& other) {
    sigs_.insert(other.sigs_.begin(), other.sigs_.end());
    return true;
  }

  std::pair<Address, Signature> getFirstValidation() {
    auto x = sigs_.begin();
    std::pair<Address, Signature> pair(x->first, x->second);
    return pair;
  }

/** Returns a JSON string representing this validation block.
 *  @return a JSON string representing this validation block.
*/
  std::string getJSON() const {
    MTR_SCOPE_FUNC();
    std::string out("[");
    bool isFirst = true;
    for (auto const& item : sigs_) {
      if (isFirst) {
        isFirst = false;
      } else {
        out += ",";
      }
      out += "\""+toHex(std::vector<byte>(std::begin(item.first)
        , std::end(item.first)))+"\":";
      out += "\""+toHex(std::vector<byte>(std::begin(item.second)
        , std::end(item.second)))+"\"";
    }
    out += "]";
    return out;
  }

/** Returns a CBOR byte vector representing this validation block.
 *  @return a CBOR byte vector representing this validation block.
*/
  std::vector<byte> getCanonical() const {
    MTR_SCOPE_FUNC();
    std::vector<byte> serial;
    for (auto const& item : sigs_) {
      serial.insert(serial.end(), item.first.begin(), item.first.end());
      serial.insert(serial.end(), item.second.begin(), item.second.end());
    }
    return serial;
  }

/** Returns the size of this verification block in bytes
 *  @return the size of this verification block in bytes
*/
  unsigned int GetByteSize() const {
    return getCanonical().size();
  }

/** Returns the number of validations in this block.
 *  @return the number of validations in this block.
*/
  unsigned int GetValidationCount() const {
    return sigs_.size();
  }

/** Returns the hash of this validation block.
 *  @return the hash of this validation block.
*/
  const Hash GetHash() const {
      return dcHash(getCanonical());
  }

  static size_t PairSize() {
    return kADDR_SIZE+kSIG_SIZE;
  }

};

} //end namespace Devcash

#endif /* SRC_PRIMITIVES_VALIDATION_H_ */
