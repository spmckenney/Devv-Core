/*
 * Signature.h - structure to represent digital signatures
 * has a 1 byte prefix indicating the signature type.
 * Currently only 72 byte secp256k1 and
 *  103 byte secp384r1 are allowed.
 *  Constructor works with or without the 1 byte type prefix.
 *
 *  Created on: July 15, 2018
 *      Author: Nick Williams
 */

#ifndef PRIMITIVES_SIGNATURE_H_
#define PRIMITIVES_SIGNATURE_H_

#include <stdint.h>
#include <algorithm>

namespace Devcash {

static const size_t kWALLET_SIG_SIZE = 72;
static const size_t kNODE_SIG_SIZE = 103;

class Signature {
 public:
  Signature() : canonical_(1, 0) {}
  /**
   *
   * @param vec
   */
  Signature(const std::vector<byte>& vec)
      : canonical_(vec) {
    if (vec.size() == kWALLET_SIG_SIZE) {
	  //prepend type
      canonical_.insert(std::begin(canonical_), kWALLET_SIG_SIZE);
    } else if (vec.size() == kWALLET_SIG_SIZE+1
               && vec.at(0) == kWALLET_SIG_SIZE) {
      //already good, copied from argument
	} else if (vec.size() == kNODE_SIG_SIZE) {
      //prepend type
      canonical_.insert(std::begin(canonical_), kNODE_SIG_SIZE);
	} else if (vec.size() == kNODE_SIG_SIZE+1
	           && vec.at(0) == kNODE_SIG_SIZE) {
      //already good, copied from argument
    } else {
	  LOG_ERROR << "Invalid Signature size.";
    }
  }

  /**
   *
   * @param other
   */
  Signature(const Signature& other) = default; // : canonical_(other.canonical_) {}

  /** Compare signatures */
  friend bool operator==(const Signature& a, const Signature& b) { return (a.canonical_ == b.canonical_); }
  /** Compare signatures */
  friend bool operator!=(const Signature& a, const Signature& b) { return (a.canonical_ != b.canonical_); }
  friend bool operator<(const Signature& a, const Signature& b) { return (a.canonical_ < b.canonical_); }
  friend bool operator<=(const Signature& a, const Signature& b) { return (a.canonical_ <= b.canonical_); }
  friend bool operator>(const Signature& a, const Signature& b) { return (a.canonical_ > b.canonical_); }
  friend bool operator>=(const Signature& a, const Signature& b) { return (a.canonical_ >= b.canonical_); }

  /** Assign signature */
  Signature& operator=(const Signature&& other) {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return *this;
  }

  /** Assign address */
  Signature& operator=(const Signature& other) {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return *this;
  }

  /**
   * Return the size of this Signature
   * @return size of this Signature
   */
  size_t size() { return canonical_.size(); }

  /**
   * Gets this signature in a canonical form.
   * @return a vector defining this transaction in canonical form.
   */
  const std::vector<byte>& getCanonical() const { return canonical_; }

  /**
   * Return the JSON representation of this Signature
   * @return hex string of this Signature
   */
  std::string getJSON() const {
    MTR_SCOPE_FUNC();
    if (isNull()) return "";
    const char alpha[] = "0123456789ABCDEF";
    std::stringstream ss;
    for (size_t j = 0; j < canonical_.size(); j++) {
      int c = (int)canonical_.at(j);
      ss.put(alpha[(c >> 4) & 0xF]);
      ss.put(alpha[c & 0xF]);
    }
    return (ss.str());
  }

  /**
   * @return type byte for this signature
   */
  byte getSignatureType() {
    return canonical_.at(0);
  }

  /**
   * Check if Signature is a wallet Signature
   * @return true iff Signature is wallet signature (72 bytes)
   * @return false otherwise
   */
  bool isWalletSignature() const {
    if (isNull()) return false;
    return (canonical_.at(0) == kWALLET_SIG_SIZE);
  }

  /**
   * Check if Signature is a node Signature
   * @return true iff Address is node address (103 bytes)
   * @return false otherwise
   */
  bool isNodeSignature() const {
    if (isNull()) return false;
    return (canonical_.at(0) == kNODE_SIG_SIZE);
  }

  bool isNull() const {
    if (canonical_.empty()) return true;
    return (canonical_.at(0) == 0);
  }

  /**
   * @return this signature without the type prefix.
   */
  std::vector<byte> getRawSignature() const {
    if (isNull()) throw std::runtime_error("Signature is not initialized!");
	std::vector<byte> raw(canonical_.begin()+1, canonical_.end());
    return raw;
  }

 private:
  /// The canonical representation of this Signature
  std::vector<byte> canonical_;
};

}  // end namespace Devcash

#endif /* PRIMITIVES_SIGNATURE_H_ */
