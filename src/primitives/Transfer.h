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
#include "common/ossladapter.h"
#include "common/util.h"
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

  Transfer(const std::vector<byte>& addr, uint64_t coin, uint64_t amount
    , uint64_t delay) : addr_(), coin_(coin), amount_(amount)
    , delay_(delay) {
    if (addr.size() != 33) {
      LOG_WARNING << "Invalid address in Transfer constructor!";
      return;
    }
    for (unsigned int i=0; i<33; ++i) {
      addr_[i] = addr.at(i);
    }
  }

  explicit Transfer(const std::vector<byte>& serial)
    : addr_(), coin_(0), amount_(0), delay_(0) {
    if (serial.size() != Size()) {
      LOG_WARNING << "Invalid serialized transfer!";
      return;
    }
    for (unsigned int i=0; i<33; ++i) {
      addr_[i] = serial.at(i);
    }
    BinToUint64(serial, 33, coin_);
    BinToInt64(serial, 41, amount_);
    BinToUint64(serial, 49, delay_);
  }

  explicit Transfer(const std::vector<byte>& serial, size_t pos)
    : addr_(), coin_(0), amount_(0), delay_(0) {
    if (serial.size() < Size()+pos) {
      LOG_WARNING << "Invalid serialized transfer!";
      return;
    }
    for (size_t i=pos; i<(33+pos); ++i) {
      addr_[i-pos] = serial.at(i);
    }
    BinToUint64(serial, 33+pos, coin_);
    BinToInt64(serial, 41+pos, amount_);
    BinToUint64(serial, 49+pos, delay_);
  }

  explicit Transfer(const Transfer &other) : addr_()
    , coin_(other.coin_), amount_(other.amount_)
    , delay_(other.delay_) {
    for (unsigned int i=0; i<33; ++i) {
      addr_[i] = other.addr_[i];
    }
  }

  static const size_t Size() {
    return 57;
  }

/** Gets this transfer in a canonical form.
 * @return a vector defining this transaction in canonical form.
 */
  std::vector<byte> getCanonical() const {
    std::vector<byte> serial(addr_, addr_+33);
    Uint64ToBin(coin_, serial);
    Int64ToBin(amount_, serial);
    Uint64ToBin(delay_, serial);
    return serial;
  }

  std::string getJSON() const {
    std::string json("{\""+kADDR_TAG+"\":\"");
    json += toHex(&addr_[0], 33);
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
      for (unsigned int i=0; i<33; ++i) {
        this->addr_[i] = other.addr_[i];
      }
      this->amount_ = other.amount_;
      this->delay_ = other.delay_;
      this->coin_ = other.coin_;
    }
    return this;
  }
  Transfer* operator=(const Transfer& other)
  {
    if (this != &other) {
      for (unsigned int i=0; i<33; ++i) {
        this->addr_[i] = other.addr_[i];
      }
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
