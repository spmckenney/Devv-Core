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
  Address addr_;
  uint64_t coin_;
  int64_t amount_;
  uint64_t delay_;

/** Constructors */
  Transfer() : addr_(), coin_(0), amount_(0), delay_(0) {}

  Transfer(const Address& addr, uint64_t coin, uint64_t amount
    , uint64_t delay) : addr_(addr), coin_(coin), amount_(amount)
    , delay_(delay) {
  }

  explicit Transfer(const std::vector<byte>& serial)
    : addr_(), coin_(0), amount_(0), delay_(0) {
    if (serial.size() != Size()) {
      LOG_WARNING << "Invalid serialized transfer!";
      return;
    }
    std::copy_n(serial.begin(), kADDR_SIZE, addr_.begin());
    coin_ = BinToUint64(serial, kADDR_SIZE);
    amount_ = BinToInt64(serial, kADDR_SIZE+8);
    delay_ = BinToUint64(serial, kADDR_SIZE+16);
  }

  explicit Transfer(const std::vector<byte>& serial, size_t pos)
    : addr_(), coin_(BinToUint64(serial, kADDR_SIZE+pos))
    , amount_(BinToInt64(serial, kADDR_SIZE+8+pos))
    , delay_(BinToUint64(serial, kADDR_SIZE+16+pos)) {
    if (serial.size() < Size()+pos) {
      LOG_WARNING << "Invalid serialized transfer!";
      return;
    }
    std::copy_n(serial.begin()+pos, kADDR_SIZE, addr_.begin());
  }

  explicit Transfer(const Transfer &other) : addr_(other.addr_)
    , coin_(other.coin_), amount_(other.amount_)
    , delay_(other.delay_) {
  }

  static const size_t Size() {
    return kADDR_SIZE+24;
  }

/** Gets this transfer in a canonical form.
 * @return a vector defining this transaction in canonical form.
 */
  std::vector<byte> getCanonical() const {
    std::vector<byte> serial(std::begin(addr_), std::end(addr_));
    Uint64ToBin(coin_, serial);
    Int64ToBin(amount_, serial);
    Uint64ToBin(delay_, serial);
    return serial;
  }

  std::string getJSON() const {
    std::string json("{\""+kADDR_TAG+"\":\"");
    json += toHex(std::vector<byte>(std::begin(addr_), std::end(addr_)));
    json += "\",\""+kTYPE_TAG+"\":"+std::to_string(coin_);
    json += ",\""+kAMOUNT_TAG+"\":"+std::to_string(amount_);
    json += ",\""+kDELAY_TAG+"\":"+std::to_string(delay_);
    json += "}";
    return json;
  }

/** Compare transfers */
  friend bool operator==(const Transfer& a, const Transfer& b)
  {
      return (a.addr_ == b.addr_ && a.amount_ == b.amount_ && a.delay_ == b.delay_);
  }

  friend bool operator!=(const Transfer& a, const Transfer& b)
  {
    return !(a.addr_ == b.addr_ && a.amount_ == b.amount_ && a.delay_ == b.delay_);
  }

/** Assign transfers */
  Transfer* operator=(Transfer&& other)
  {
    if (this != &other) {
      addr_ = other.addr_;
      this->amount_ = other.amount_;
      this->delay_ = other.delay_;
      this->coin_ = other.coin_;
    }
    return this;
  }
  Transfer* operator=(const Transfer& other)
  {
    if (this != &other) {
      addr_ = other.addr_;
      this->amount_ = other.amount_;
      this->delay_ = other.delay_;
      this->coin_ = other.coin_;
    }
    return this;
  }

  /** Returns the delay in seconds until this transfer is final.
   * @return the delay in seconds until this transfer is final.
  */
    uint64_t getDelay() const {
      return delay_;
    }
};

} //end namespace Devcash


#endif /* PRIMITIVES_TRANSFER_H_ */
