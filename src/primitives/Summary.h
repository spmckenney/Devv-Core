/*
 * Summary.h
 *
 * Serial form is Address count (uint64_t),...
 * [Address count times]:
 *   Address,delayed coin count (uint64_t), coin count (uint64_t)...
 *     coin, delay, delta [delayed coin count times]
 *     coin, delta [coin count times]
 *
 *  Created on: Apr 18, 2018
 *      Author: Nick Williams
 */

#ifndef PRIMITIVES_SUMMARY_H_
#define PRIMITIVES_SUMMARY_H_

#include <stdint.h>
#include <mutex>
#include <map>
#include <vector>

#include "Transfer.h"

namespace Devcash
{

struct DelayedItem {
  uint64_t delay=0;
  uint64_t delta=0;
  DelayedItem(uint64_t delay=0, uint64_t delta=0) : delay(delay)
    , delta(delta) {}
};

static const std::string kADDR_SIZE_TAG = "addr_size";
static const std::string kDELAY_SIZE_TAG = "delay_size";
static const std::string kCOIN_SIZE_TAG = "coin_size";

typedef std::map<uint64_t, DelayedItem> DelayedMap;
typedef std::map<uint64_t, uint64_t> CoinMap;
typedef std::pair<DelayedMap, CoinMap> SummaryPair;
typedef std::map<Address, SummaryPair> SummaryMap;

class Summary {
 public:
  SummaryMap summary_;
  //std::mutex lock_;

  /** Constrcutors */
  Summary() {}
  Summary(SummaryMap summary) : summary_(summary) {}
  Summary(const Summary& other) : summary_(other.summary_) {}
  Summary(const std::vector<byte>& serial, size_t& offset) : summary_() {
    if (serial.size() < MinSize()+offset) {
      LOG_WARNING << "Invalid serialized Summary, too small!";
      return;
    }
    size_t addr_count = BinToUint32(serial, offset);
    offset += 4;
    std::vector<byte> out;
    for (size_t i=0; i<addr_count; ++i) {
      Address one_addr;
      std::copy_n(serial.begin()+offset, kADDR_SIZE, one_addr.begin());
      offset += kADDR_SIZE;
      DelayedMap delayed;
      CoinMap coin_map;
      size_t delayed_count = BinToUint64(serial, offset);
      offset += 8;
      size_t coin_count = BinToUint64(serial, offset);
      offset += 8;
      for (size_t j=0; j<delayed_count; ++j) {
        uint64_t coin = BinToUint64(serial, offset);
        offset += 8;
        uint64_t delay = BinToUint64(serial, offset);
        offset += 8;
        uint64_t delta = BinToUint64(serial, offset);
        offset += 8;
        DelayedItem d_item(delay, delta);
        std::pair<uint64_t, DelayedItem> new_pair(coin, d_item);
        delayed.insert(new_pair);
      }
      for (size_t j=0; j<coin_count; ++j) {
        uint64_t coin = BinToUint64(serial, offset);
        offset += 8;
        uint64_t delta = BinToUint64(serial, offset);
        offset += 8;
        std::pair<uint64_t, uint64_t> new_pair(coin, delta);
        coin_map.insert(new_pair);
      }
      SummaryPair top_pair(delayed, coin_map);
      summary_.insert(std::pair<Address, SummaryPair>(one_addr, top_pair));
    }
  }

  /** Adds a summary record to this block.
   *  @param addr the address that this change applies to
   *  @param coin the coin type that this change applies to
   *  @param item a chain state vector summary of transactions.
   *  @return true iff the summary was added
  */
  bool addItem(const Address& addr, uint64_t coin, const DelayedItem& item) {
    CASH_TRY {
      //std::lock_guard<std::mutex> lock(lock_);
      if (summary_.count(addr) > 0) {
        SummaryPair existing(summary_.at(addr));
        if (item.delay > 0) {
          DelayedMap delayed(existing.first);
          if (!addToDelayedMap(coin, item, delayed)) return false;
          SummaryPair updated(delayed, existing.second);
          summary_.at(addr) = updated;
        } else {
          CoinMap the_map(existing.second);
          if (!addToCoinMap(coin, item, the_map)) return false;
          SummaryPair updated(existing.first, the_map);
          summary_.at(addr) = updated;
        }
      } else {
        DelayedMap new_delayed;
        CoinMap new_map;
        if (item.delay > 0) {
          std::pair<uint64_t, DelayedItem> new_pair(coin, item);
          new_delayed.insert(new_pair);
        } else {
          std::pair<uint64_t, uint64_t> new_pair(coin, item.delta);
          new_map.insert(new_pair);
        }
        SummaryPair new_summary(new_delayed, new_map);
        summary_.insert(std::pair<Address, SummaryPair>(addr, new_summary));
      }
      return true;
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "summary");
      return false;
    }
  }

  /** Adds a summary record to this block.
   *  @param addr the addresses involved in this change
   *  @param coinType the type number for this coin
   *  @param delta change in coins (>0 for receiving, <0 for spending)
   *  @param delay the delay in seconds before this transaction can be received
   *  @return true iff the summary was added
  */
  bool addItem(const Address& addr, uint64_t coinType, uint64_t delta
      , uint64_t delay=0) {
    DelayedItem item(delay, delta);
    return addItem(addr, coinType, item);
  }

  /**
   *  @return a canonical bytestring summarizing these changes.
  */
  std::vector<byte> getCanonical() const {
    std::vector<byte> out;
    uint32_t addr_count = summary_.size();
    Uint32ToBin(addr_count, out);
    for (auto iter = summary_.begin(); iter != summary_.end(); ++iter) {
      out.insert(out.end(), iter->first.begin(), iter->first.end());

      SummaryPair top_pair(iter->second);
      DelayedMap delayed(top_pair.first);
      CoinMap coin_map(top_pair.second);
      uint64_t delayed_count = delayed.size();
      Uint64ToBin(delayed_count, out);
      uint64_t coin_count = coin_map.size();
      Uint64ToBin(coin_count, out);
      if (!delayed.empty()) {
        for (auto j = delayed.begin(); j != delayed.end(); ++j) {
          Uint64ToBin(j->first, out);
          Uint64ToBin(j->second.delay, out);
          Uint64ToBin(j->second.delta, out);
        }
      }

      if (!coin_map.empty()) {
        for (auto j = coin_map.begin(); j != coin_map.end(); ++j) {
          Uint64ToBin(j->first, out);
          Uint64ToBin(j->second, out);
        }
      }
    }
    return out;
  }

  size_t getByteSize() const {
    return getCanonical().size();
  }

  std::string getJSON() const {
    std::string json("{\""+kADDR_SIZE_TAG+"\":");
    uint64_t addr_size = summary_.size();
    json += std::to_string(addr_size)+",summary:[";
    for (auto iter = summary_.begin(); iter != summary_.end(); ++iter) {
      json += "\""+toHex(std::vector<byte>(std::begin(iter->first)
        , std::end(iter->first)))+"\":[";
      SummaryPair top_pair(iter->second);
      DelayedMap delayed(top_pair.first);
      CoinMap coin_map(top_pair.second);
      uint64_t delayed_size = delayed.size();
      uint64_t coin_size = coin_map.size();
      json += "\""+kDELAY_SIZE_TAG+"\":"+std::to_string(delayed_size)+",";
      json += "\""+kCOIN_SIZE_TAG+"\":"+std::to_string(coin_size)+",";
      bool isFirst = true;
      json += "\"delayed\":[";
      if (!delayed.empty()) {
        for (auto j = delayed.begin(); j != delayed.end(); ++j) {
          if (isFirst) {
            isFirst = false;
          } else {
            json += ",";
          }
          json += "\""+kTYPE_TAG+"\":"+std::to_string(j->first)+",";
          json += "\""+kDELAY_TAG+"\":"+std::to_string(j->second.delay)+",";
          json += "\""+kAMOUNT_TAG+"\":"+std::to_string(j->second.delta);
        }
      }
      json += "],\"coin_map\":[";
      isFirst = true;
      if (!coin_map.empty()) {
        for (auto j = coin_map.begin(); j != coin_map.end(); ++j) {
          if (isFirst) {
            isFirst = false;
          } else {
            json += ",";
          }
          json += "\""+kTYPE_TAG+"\":"+std::to_string(j->first)+",";
          json += "\""+kAMOUNT_TAG+"\":"+std::to_string(j->second);
        }
      }
      json += "]";
    }
    json += "]}";
    return json;
  }

  /**
   *  @return true iff, the summary passes sanity checks
  */
  bool isSane() const {
    CASH_TRY {
      if (summary_.empty()) return false;
      uint64_t coinTotal = 0;
      for (auto iter = summary_.begin(); iter != summary_.end(); ++iter) {
        for (auto j = iter->second.first.begin();
            j != iter->second.first.end(); ++j) {
          coinTotal += j->second.delta;
        }
        for (auto j = iter->second.second.begin();
            j != iter->second.second.end(); ++j) {
          coinTotal += j->second;
        }
      }
      if (coinTotal != 0) {
        LOG_DEBUG << "Summary state invalid: "+getJSON();
        return false;
      }
      return true;
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "summary");
      return false;
    }
  }

  static size_t MinSize() {
    return 4;
  }

 private:
  bool addToDelayedMap(uint64_t coin, const DelayedItem& item
      , DelayedMap& existing) {
    if (existing.count(coin) > 0) {
      DelayedItem the_item = existing.at(coin);
      the_item.delta += item.delta;
      the_item.delay = std::max(the_item.delay, item.delay);
      existing.at(coin) = the_item;
    } else {
      std::pair<uint64_t, DelayedItem> one_item(coin, item);
      existing.insert(one_item);
    }
    return true;
  }

  bool addToCoinMap(uint64_t coin, const DelayedItem& item
      , CoinMap& existing) {
    if (existing.count(coin) > 0) {
      uint64_t the_item = existing.at(coin);
      the_item += item.delta;
      existing.at(coin) = the_item;
    } else {
      std::pair<uint64_t, uint64_t> one_item(coin, item.delta);
      existing.insert(one_item);
    }
    return true;
  }

};

} //end namespace Devcash

#endif /* PRIMITIVES_SUMMARY_H_ */
