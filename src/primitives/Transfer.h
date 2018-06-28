/*
 * Transfer.h - structure to represent the movement of
 * SmartCoins from one address to another
 *
 *  Created on: Apr 17, 2018
 *      Author: Nick Williams
 */

#ifndef PRIMITIVES_TRANSFER_H_
#define PRIMITIVES_TRANSFER_H_

#include <stdint.h>
#include <algorithm>
#include <string>

#include "SmartCoin.h"
#include "primitives/buffers.h"
#include "common/binary_converters.h"
#include "consensus/KeyRing.h"
#include "consensus/chainstate.h"

namespace Devcash {

/// @todo (mckenney) move to constants file
static const std::string kTYPE_TAG = "type";
static const std::string kDELAY_TAG = "dlay";
static const std::string kADDR_TAG = "addr";
static const std::string kAMOUNT_TAG = "amount";

/**
 * A single coin transfer
 */
class Transfer {
 public:
  /**
   *
   * @param addr
   * @param coin
   * @param amount
   * @param delay
   */
  Transfer(const Address& addr,
           uint64_t coin,
           int64_t amount,
           uint64_t delay)
      : canonical_(std::begin(addr), std::end(addr)) {
    Uint64ToBin(coin, canonical_);
    Int64ToBin(amount, canonical_);
    Uint64ToBin(delay, canonical_);
  }

  /**
   * Create a transfer from the buffer serial and update the offset
   * @param[in] serial
   * @param[in, out] offset
   */
  Transfer(InputBuffer& buffer)
      : canonical_(buffer.getCurrentIterator(), buffer.getCurrentIterator() + Size()) {
    if (buffer.size() < Size() + buffer.getOffset()) {
      LOG_WARNING << "Invalid serialized transfer!";
      return;
    }
    buffer.increment(Size());
  }

  /**
   *
   * @param other
   */
  Transfer(const Transfer& other) = default; // : canonical_(other.canonical_) {}

  /** Compare transfers */
  friend bool operator==(const Transfer& a, const Transfer& b) { return (a.canonical_ == b.canonical_); }
  /** Compare transfers */
  friend bool operator!=(const Transfer& a, const Transfer& b) { return (a.canonical_ != b.canonical_); }

  /** Assign transfers */
  Transfer& operator=(const Transfer&& other) {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return *this;
  }

  /** Assign transfers */
  Transfer& operator=(const Transfer& other) {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return *this;
  }

  /**
   * Return the size of this Transfer
   * @return size of this Transfer
   * @todo (mckenney) move to constants
   */
  static size_t Size() { return kADDR_SIZE + 24; }

  /**
   * Gets this transfer in a canonical form.
   * @return a vector defining this transaction in canonical form.
   */
  const std::vector<byte>& getCanonical() const { return canonical_; }

  /**
   * Return the JSON representation of this Transfer
   * @return
   */
  std::string getJSON() const {
    std::string json("{\"" + kADDR_TAG + "\":\"");
    Address addr = getAddress();
    json += ToHex(std::vector<byte>(std::begin(addr), std::end(addr)));
    json += "\",\"" + kTYPE_TAG + "\":" + std::to_string(getCoin());
    json += ",\"" + kAMOUNT_TAG + "\":" + std::to_string(getAmount());
    json += ",\"" + kDELAY_TAG + "\":" + std::to_string(getDelay());
    json += "}";
    return json;
  }

  /**
   * Get the address of this coin
   * @return
   */
  Address getAddress() const {
    Address addr;
    std::copy_n(canonical_.begin(), kADDR_SIZE, addr.begin());
    return addr;
  }

  /**
   * Return the coin
   * @return
   */
  uint64_t getCoin() const { return BinToUint64(canonical_, kADDR_SIZE); }

  /**
   * Return the amount of this Transfer
   * @return
   */
  int64_t getAmount() const { return BinToInt64(canonical_, kADDR_SIZE + 8); }

  /**
   * Returns the delay in seconds until this transfer is final.
   * @return the delay in seconds until this transfer is final.
   */
  uint64_t getDelay() const { return BinToUint64(canonical_, kADDR_SIZE + 16); }

 private:
  /// The canonical representation of this Transfer
  std::vector<byte> canonical_;
};

/**
 * Converts the vector of bytes to an array of bytes (Devcash::Address)
 * @param[in] vec input address as vector of bytes
 * @return The resulting Devcash::Address created from the input vector
 */
static Devcash::Address ConvertToAddress(const std::vector<byte>& vec) {
  CheckSizeEqual(vec, Devcash::kADDR_SIZE);
  Devcash::Address addr;
  std::copy_n(vec.begin(), Devcash::kADDR_SIZE, addr.begin());
  return(addr);
}

}  // end namespace Devcash

#endif /* PRIMITIVES_TRANSFER_H_ */
