/*
 * Address.h - structure to represent public keys
 * has a 1 byte prefix indicating the key type.
 * Currently only 33 byte secp256k1 and
 *  50 byte secp384r1 are allowed.
 *  Constructor works with or without the 1 byte type prefix.
 *
 *  Created on: July 13, 2018
 *      Author: Nick Williams
 */

#ifndef PRIMITIVES_ADDRESS_H_
#define PRIMITIVES_ADDRESS_H_

#include <stdint.h>
#include <algorithm>

namespace Devcash {

static const size_t kWALLET_ADDR_SIZE = 33;
static const size_t kNODE_ADDR_SIZE = 50;

class Address {
 public:
  /**
   *
   * @param vec
   */
  Address(const std::vector<byte>& vec)
      : canonical_(vec.size()) {
    if (vec.size() == kWALLET_ADDR_SIZE) {
      canonical_.insert(std::end(canonical_),std::begin(vec),std::end(vec));
    } else if (vec.size() == kWALLET_ADDR_SIZE+1
               && vec.at(0) == kWALLET_ADDR_SIZE) {
      canonical_.insert(std::end(canonical_),std::begin(vec)+1,std::end(vec));
	} else if (vec.size() == kNODE_ADDR_SIZE) {
      canonical_.insert(std::end(canonical_),std::begin(vec),std::end(vec));
	} else if (vec.size() == kNODE_ADDR_SIZE+1
	           && vec.at(0) == kNODE_ADDR_SIZE) {
      canonical_.insert(std::end(canonical_),std::begin(vec)+1,std::end(vec));
    } else {
	  std::string err = "Invalid Address size.";
	  throw std::runtime_error(err);
    }
  }

  /**
   *
   * @param other
   */
  Address(const Address& other) = default; // : canonical_(other.canonical_) {}

  /** Compare addresses */
  friend bool operator==(const Address& a, const Address& b) { return (a.canonical_ == b.canonical_); }
  /** Compare addresses */
  friend bool operator!=(const Address& a, const Address& b) { return (a.canonical_ != b.canonical_); }
  friend bool operator<(const Address& a, const Address& b) { return (a.canonical_ < b.canonical_); }
  friend bool operator<=(const Address& a, const Address& b) { return (a.canonical_ <= b.canonical_); }
  friend bool operator>(const Address& a, const Address& b) { return (a.canonical_ > b.canonical_); }
  friend bool operator>=(const Address& a, const Address& b) { return (a.canonical_ >= b.canonical_); }

  /** Assign address */
  Address& operator=(const Address&& other) {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return *this;
  }

  /** Assign address */
  Address& operator=(const Address& other) {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return *this;
  }

  /**
   * Return the size of this Address
   * @return size of this Address
   */
  static size_t size() { return canonical_.size(); }

  /**
   * Gets this address in a canonical form.
   * @return a vector defining this transaction in canonical form.
   */
  const std::vector<byte>& getCanonical() const { return canonical_; }

  /**
   * Return the JSON representation of this Address
   * @return hex string of this Address
   */
  std::string getJSON() const {
    return ToHex(canonical_);
  }

  /**
   * @return type byte for this address
   */
  byte getAddressType() {
    return canonical_.at(0);
  }

  /**
   * Check if Address is a wallet Address
   * @return true iff Address is wallet address (34 bytes)
   * @return false otherwise
   */
  bool isWalletAddress() const {
    return (canonical_.at(0) == kWALLET_ADDR_SIZE);
  }

  /**
   * Check if Address is a node Address
   * @return true iff Address is node address (51 bytes)
   * @return false otherwise
   */
  bool isNodeAddress() const {
    return (canonical_.at(0) == kNODE_ADDR_SIZE);
  }

  std::vector<byte> getAddressRaw() {
	std::vector<byte> raw(canonical_.begin()+1, canonical_.end());
    return raw;
  }

 private:
  /// The canonical representation of this Address
  std::vector<byte> canonical_;
};

}  // end namespace Devcash

#endif /* PRIMITIVES_TRANSFER_H_ */
