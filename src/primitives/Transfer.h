/*
 * Transfer.h - structure to represent the movement of
 * SmartCoins from one address to another
 *
 *  Created on: Apr 17, 2018
 *      Author: Nick Williams
 */

#ifndef PRIMITIVES_TRANSFER_H_
#define PRIMITIVES_TRANSFER_H_

#include <string>
#include <stdint.h>

#include "smartcoin.h"
#include "consensus/KeyRing.h"
#include "consensus/chainstate.h"

namespace Devcash
{

static const std::string kTYPE_TAG = "type";
static const std::string kDELAY_TAG = "dlay";
static const std::string kADDR_TAG = "addr";
static const std::string kAMOUNT_TAG = "amount";

class Transfer {
 public:

/** Constructors */
  Transfer() : canonical_() {}

  Transfer(const Address& addr, uint64_t coin, uint64_t amount
    , uint64_t delay)
    : canonical_(std::begin(addr), std::end(addr)) {
    Uint64ToBin(coin, canonical_);
    Int64ToBin(amount, canonical_);
    Uint64ToBin(delay, canonical_);
  }

  explicit Transfer(const std::vector<byte>& serial)
    : canonical_(serial) {
    if (serial.size() != Size()) {
      LOG_WARNING << "Invalid serialized transfer!";
      return;
    }
  }

  explicit Transfer(const std::vector<byte>& serial, size_t& offset)
    : canonical_(serial.begin()+offset, serial.begin()+offset+Size()) {
    if (serial.size() < Size()+offset) {
      LOG_WARNING << "Invalid serialized transfer!";
      return;
    }
    offset += Size();
  }

  explicit Transfer(const Transfer &other) : canonical_(other.canonical_) {}

  static const size_t Size() {
    return kADDR_SIZE+24;
  }

/** Gets this transfer in a canonical form.
 * @return a vector defining this transaction in canonical form.
 */
  std::vector<byte> getCanonical() const {
    return canonical_;
  }

  std::string getJSON() const {
    std::string json("{\""+kADDR_TAG+"\":\"");
    Address addr = getAddress();
    json += toHex(std::vector<byte>(std::begin(addr), std::end(addr)));
    json += "\",\""+kTYPE_TAG+"\":"+std::to_string(getCoin());
    json += ",\""+kAMOUNT_TAG+"\":"+std::to_string(getAmount());
    json += ",\""+kDELAY_TAG+"\":"+std::to_string(getDelay());
    json += "}";
    return json;
  }

/** Compare transfers */
  friend bool operator==(const Transfer& a, const Transfer& b)
  {
    return (a.canonical_ == b.canonical_);
  }

  friend bool operator!=(const Transfer& a, const Transfer& b)
  {
    return (a.canonical_ != b.canonical_);
  }

/** Assign transfers */
  Transfer* operator=(Transfer&& other)
  {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return this;
  }
  Transfer* operator=(const Transfer& other)
  {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return this;
  }

  Address getAddress() const {
    Address addr;
    std::copy_n(canonical_.begin(), kADDR_SIZE, addr.begin());
    return addr;
  }

  uint64_t getCoin() const {
    return BinToUint64(canonical_, kADDR_SIZE);
  }

  int64_t getAmount() const {
    return BinToInt64(canonical_, kADDR_SIZE+8);
  }

  /** Returns the delay in seconds until this transfer is final.
   * @return the delay in seconds until this transfer is final.
  */
  uint64_t getDelay() const {
    return BinToUint64(canonical_, kADDR_SIZE+16);
  }

 private:
  std::vector<byte> canonical_;
};

} //end namespace Devcash


#endif /* PRIMITIVES_TRANSFER_H_ */
