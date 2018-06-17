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

#include "SmartCoin.h"

namespace Devcash {

/// A map of validations - Addresses and Signatures
typedef std::map<Address, Signature> ValidationMap;

/**
 * A validation.
 */
class Validation {
 public:
  /**
   * Default constructor
   */
  Validation() = default;

  /**
   * Constructor. Deserializes from serial and updates the offset
   * @param[in] serial
   * @param[in, out] offset
   */
  Validation(InputBuffer& buffer) : sigs_() {
    MTR_SCOPE_FUNC();
    size_t remainder = buffer.size() - buffer.getOffset();
    while (remainder >= pairSize()) {
      Address one_addr;
      Signature one_sig;
      buffer.copy(one_addr);
      buffer.copy(one_sig);
      std::pair<Address, Signature> one_pair(one_addr, one_sig);
      sigs_.insert(one_pair);
      remainder -= pairSize();
    }
  }

  /**
   * Constructor
   * @param[in] serial
   * @param[in, out] offset
   * @param[in] count
   */
  Validation(InputBuffer& buffer, uint32_t count) : sigs_() {
    MTR_SCOPE_FUNC();
    if (buffer.size() < buffer.getOffset() + (count * pairSize())) {
      LOG_WARNING << "Invalid Validation, too small";
    }
    size_t remainder = count;
    while (remainder > 0) {
      Address one_addr;
      buffer.copy(one_addr);
      Signature one_sig;
      buffer.copy(one_sig);
      std::pair<Address, Signature> one_pair(one_addr, one_sig);
      sigs_.insert(one_pair);
      remainder--;
    }
  }

  /**
   * Constructor
   * @param other
   */
  Validation(const Validation& other) = default; //: sigs_(other.sigs_) {}

  /**
   * Constructor
   * @param[in] node
   * @param[in] sig
   */
  Validation(const Address& node, const Signature& sig) : sigs_() { sigs_.insert(std::pair<Address, Signature>(node, sig)); }

  /**
   * Constructor
   * @param validation_map
   */
  explicit Validation(const ValidationMap& validation_map) : sigs_(validation_map) {}

  /** Adds a validation record to this block.
   *  @param node the address of node that produced this validation
   *  @param sig the signature of the validating node
   *  @return true iff the validation verified against the node's public key
   */
  bool addValidation(const Address& node, const Signature& sig) {
    sigs_.insert(std::pair<Address, Signature>(node, sig));
    return true;
  }

  /**
   * Add other to the ValidationMap
   *
   * @param other
   * @return
   */
  bool addValidation(const Validation& other) {
    sigs_.insert(other.sigs_.begin(), other.sigs_.end());
    return true;
  }

  /**
   * Return the first validation
   * @return
   */
  std::pair<Address, Signature> getFirstValidation() {
    auto x = sigs_.begin();
    std::pair<Address, Signature> pair(x->first, x->second);
    return pair;
  }

  /**
   * Returns a CBOR byte vector representing this validation block.
   * @return a CBOR byte vector representing this validation block.
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

  /**
   * Returns the size of this verification block in bytes
   * @return the size of this verification block in bytes
   */
  unsigned int getByteSize() const { return getCanonical().size(); }

  /**
   * Returns the number of validations in this block.
   * @return the number of validations in this block.
   */
  unsigned int getValidationCount() const { return sigs_.size(); }

  /**
   * Returns the hash of this validation block.
   * @return the hash of this validation block.
   */
  const Hash getHash() const { return DevcashHash(getCanonical()); }

  /**
   * Return the size of a pair (Address + Signature)
   * @return
   */
  static size_t pairSize() { return kADDR_SIZE + kSIG_SIZE; }

  /**
   * Get a reference to the ValidationMap
   * @return
   */
  const ValidationMap& getValidationMap() const { return sigs_; }

 private:
  /// Map of signatures
  ValidationMap sigs_;
};

}  // end namespace Devcash

#endif /* SRC_PRIMITIVES_VALIDATION_H_ */
