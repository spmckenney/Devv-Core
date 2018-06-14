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
#include <map>
#include <mutex>
#include <vector>

#include "Transfer.h"

namespace Devcash {

/**
 * Holds the change and delay of a coin
 */
struct DelayedItem {
  /**
   * Constructor
   * @param delay
   * @param delta
   */
  explicit DelayedItem(uint64_t delay = 0, uint64_t delta = 0)
      : delay(delay), delta(delta) {}

  /// The delay of this item
  uint64_t delay = 0;
  /// The coin/transfer price (change)
  uint64_t delta = 0;
};

static const std::string kADDR_SIZE_TAG = "addr_size";
static const std::string kDELAY_SIZE_TAG = "delay_size";
static const std::string kCOIN_SIZE_TAG = "coin_size";

typedef std::map<uint64_t, DelayedItem> DelayedMap;
typedef std::map<uint64_t, uint64_t> CoinMap;
typedef std::pair<DelayedMap, CoinMap> SummaryPair;
typedef std::map<Address, SummaryPair> SummaryMap;

/**
 * Add a coin to the existing map
 * @param[in] coin The coin to add to
 * @param[in] item
 * @param[in,out] existing
 * @return
 */
inline bool AddToDelayedMap(uint64_t coin, const DelayedItem &item, DelayedMap &existing) {
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

/**
 * Add a coin to the existing map
 * @param[in] coin The coin to add to
 * @param[in] item
 * @param[in,out] existing
 * @return
 */
inline bool AddToCoinMap(uint64_t coin, const DelayedItem& item, CoinMap& existing) {
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

/**
 * Interface to a summary map of addresses and coins.
 */
class Summary {
 public:
  /**
   * Move constructor
   * @param other
   */
  Summary(Summary&& other) noexcept = default;

  /**
   * Default move-assignment operator
   * @param other
   * @return
   */
  Summary& operator=(Summary&& other) = default;

  /**
   * Create a Summary from an input data stream
   * @param serial
   * @param offset
   * @return
   */
  static Summary Create(const std::vector<byte>& serial, size_t& offset);

  /**
   * Create an empty Summary
   * @return
   */
  static Summary Create();

  /**
   * Perform a deep copy
   * @param summary
   * @return
   */
  static Summary Copy(const Summary& summary);

  /**
   * Minimum size of Summary
   * @return Minimum size in bytes (4)
   */
  static size_t MinSize() { return 4; }

  /**
   * Adds a summary record to this block.
   * @param[in] addr the address that this change applies to
   * @param[in] coin the coin type that this change applies to
   * @param[in] item a chain state vector summary of transactions.
   * @return true iff the summary was added
   */
  bool addItem(const Address &addr, uint64_t coin, const DelayedItem &item) {
    // std::lock_guard<std::mutex> lock(lock_);
    if (summary_.count(addr) > 0) {
      SummaryPair existing(summary_.at(addr));
      if (item.delay > 0) {
        DelayedMap delayed(existing.first);
        if (!AddToDelayedMap(coin, item, delayed)) return false;
        SummaryPair updated(delayed, existing.second);
        summary_.at(addr) = updated;
      } else {
        CoinMap the_map(existing.second);
        if (!AddToCoinMap(coin, item, the_map)) return false;
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
  }

  /**
   * Adds a summary record to this block.
   * @param[in] addr the addresses involved in this change
   * @param[in] coin_type the type number for this coin
   * @param[in] delta change in coins (>0 for receiving, <0 for spending)
   * @param[in] delay the delay in seconds before this transaction can be received
   * @return true iff the summary was added
   */
  bool addItem(const Address& addr, uint64_t coin_type, uint64_t delta, uint64_t delay = 0) {
    DelayedItem item(delay, delta);
    return addItem(addr, coin_type, item);
  }

  /**
   * Get the canonical form of this summary.
   * @return a canonical bytestring summarizing these changes.
   */
  std::vector<byte> getCanonical() const {
    /// @todo (mckenney) optimization
    std::vector<byte> out;
    auto addr_count = static_cast<uint32_t>(summary_.size());
    Uint32ToBin(addr_count, out);
    for (auto summary : summary_) {
      out.insert(out.end(), summary.first.begin(), summary.first.end());

      SummaryPair top_pair(summary.second);
      DelayedMap delayed(top_pair.first);
      CoinMap coin_map(top_pair.second);
      uint64_t delayed_count = delayed.size();
      Uint64ToBin(delayed_count, out);
      uint64_t coin_count = coin_map.size();
      Uint64ToBin(coin_count, out);
      for (auto delayed_item : delayed) {
        Uint64ToBin(delayed_item.first, out);
        Uint64ToBin(delayed_item.second.delay, out);
        Uint64ToBin(delayed_item.second.delta, out);
      }
      for (auto coin : coin_map) {
        Uint64ToBin(coin.first, out);
        Uint64ToBin(coin.second, out);
      }
    }
    return out;
  }

  /**
   * Get the size of the canonical bytestring
   * @return the size of the canonical bytestring
   */
  size_t getByteSize() const { return getCanonical().size(); }

  /**
   * Get a JSON string representing this summary
   * @return JSON string
   */
  std::string getJSON() const {
    std::string json("{\"" + kADDR_SIZE_TAG + "\":");
    uint64_t addr_size = summary_.size();
    json += std::to_string(addr_size) + ",summary:[";
    for (auto summary : summary_) {
      json += "\"" + ToHex(std::vector<byte>(std::begin(summary.first), std::end(summary.first))) + "\":[";
      SummaryPair top_pair(summary.second);
      DelayedMap delayed(top_pair.first);
      CoinMap coin_map(top_pair.second);
      uint64_t delayed_size = delayed.size();
      uint64_t coin_size = coin_map.size();
      json += "\"" + kDELAY_SIZE_TAG + "\":" + std::to_string(delayed_size) + ",";
      json += "\"" + kCOIN_SIZE_TAG + "\":" + std::to_string(coin_size) + ",";
      bool is_first = true;
      json += "\"delayed\":[";
      for (auto delayed_item : delayed) {
        if (is_first) {
          is_first = false;
        } else {
          json += ",";
        }
        json += "\"" + kTYPE_TAG + "\":" + std::to_string(delayed_item.first) + ",";
        json += "\"" + kDELAY_TAG + "\":" + std::to_string(delayed_item.second.delay) + ",";
        json += "\"" + kAMOUNT_TAG + "\":" + std::to_string(delayed_item.second.delta);
      }
      json += "],\"coin_map\":[";
      is_first = true;
      for (auto coin : coin_map) {
        if (is_first) {
          is_first = false;
        } else {
          json += ",";
        }
        json += "\"" + kTYPE_TAG + "\":" + std::to_string(coin.first) + ",";
        json += "\"" + kAMOUNT_TAG + "\":" + std::to_string(coin.second);
      }
      json += "]";
    }
    json += "]}";
    return json;
  }

  /**
   * Get the transfers
   * @return a vector of Transfers
   */
  std::vector<Transfer> getTransfers() {
    std::vector<Transfer> out;
    for (auto summary : summary_) {
      SummaryPair top_pair(summary.second);
      DelayedMap delayed(top_pair.first);
      CoinMap coin_map(top_pair.second);
      for (auto delayed_item : delayed) {
        Transfer xfer(summary.first, delayed_item.first, delayed_item.second.delta, delayed_item.second.delay);
        out.push_back(xfer);
      }
      for (auto coin : coin_map) {
        Transfer xfer(summary.first, coin.first, coin.second, 0);
        out.push_back(xfer);
      }
    }
    return out;
  }

  /**
   * Return the number of transfers in this summary
   * @return number of transfers (coins and delayed coins)
   */
  size_t getTransferCount() const {
    size_t xfer_count = 0;
    for (auto summary : summary_) {
      SummaryPair top_pair(summary.second);
      DelayedMap delayed(top_pair.first);
      CoinMap coin_map(top_pair.second);
      xfer_count += delayed.size();
      xfer_count += coin_map.size();
    }
    return xfer_count;
  }

  /**
   * Get vector of coins corresponding to this address
   * @param[in] addr Address mapped to coins
   * @param[in] elapsed Include coins if delay < elapsed time in milliseconds
   * @return
   */
  std::vector<SmartCoin> getCoinsByAddr(const Address& addr, uint64_t elapsed) {
    std::vector<SmartCoin> out;
    if (summary_.count(addr) > 0) {
      SummaryPair existing(summary_.at(addr));
      DelayedMap delayed(existing.first);
      CoinMap coin_map(existing.second);
      for (auto delayed_item : delayed) {
        if (delayed_item.second.delay < elapsed) {
          SmartCoin coin(addr, delayed_item.first, delayed_item.second.delta);
          out.push_back(coin);
        }
      }
      for (auto this_coin : coin_map) {
        SmartCoin coin(addr, this_coin.first, this_coin.second);
        out.push_back(coin);
      }
    }
    return out;
  }

  /**
   * Perform sanity check and ensure summary sums to zero
   * @return true iff, the summary passes sanity checks
   */
  bool isSane() const {
    if (summary_.empty()) return false;
    uint64_t coin_total = 0;
    for (auto summary : summary_) {
      auto summary_pair = summary.second;
      auto delayed_map = summary_pair.first;
      auto coin_map = summary_pair.second;
      for (auto delayed_coin : delayed_map) {
        coin_total += delayed_coin.second.delta;
      }
      for (auto coin : coin_map) {
        coin_total += coin.second;
      }
    }
    if (coin_total != 0) {
      LOG_WARNING << "Summary state invalid: " + getJSON();
      return false;
    }
    return true;
  }

 private:
  /**
   * Constructor
   */
  Summary() noexcept = default;

  /**
   * Copy constructor
   * @param other
   */
  Summary(const Summary& other) = default;

  /// map of summary addresses to summary pairs
  SummaryMap summary_;
};

inline Summary Summary::Create() {
  Summary new_summary;
  return new_summary;
}

inline Summary Summary::Create(const std::vector<byte>& serial, size_t& offset) {
  Summary new_summary;

  if (serial.size() < Summary::MinSize() + offset) {
    std::string warning = "Invalid serialized Summary, too small!";
    LOG_WARNING << warning;
    throw std::runtime_error(warning);
  }
  size_t addr_count = BinToUint32(serial, offset);
  offset += 4;
  std::vector<byte> out;
  for (size_t i = 0; i < addr_count; ++i) {
    Address one_addr;
    std::copy_n(serial.begin() + offset, kADDR_SIZE, one_addr.begin());
    offset += kADDR_SIZE;
    DelayedMap delayed;
    CoinMap coin_map;
    size_t delayed_count = BinToUint64(serial, offset);
    offset += 8;
    size_t coin_count = BinToUint64(serial, offset);
    offset += 8;
    for (size_t j = 0; j < delayed_count; ++j) {
      uint64_t coin = BinToUint64(serial, offset);
      offset += 8;
      uint64_t delay = BinToUint64(serial, offset);
      offset += 8;
      uint64_t delta = BinToUint64(serial, offset);
      offset += 8;
      DelayedItem delayed_item(delay, delta);
      std::pair<uint64_t, DelayedItem> new_pair(coin, delayed_item);
      delayed.insert(new_pair);
    }
    for (size_t j = 0; j < coin_count; ++j) {
      uint64_t coin = BinToUint64(serial, offset);
      offset += 8;
      uint64_t delta = BinToUint64(serial, offset);
      offset += 8;
      std::pair<uint64_t, uint64_t> new_pair(coin, delta);
      coin_map.insert(new_pair);
    }
    SummaryPair top_pair(delayed, coin_map);
    new_summary.summary_.insert(std::pair<Address, SummaryPair>(one_addr, top_pair));
  }
  return new_summary;
}

inline Summary Summary::Copy(const Summary& summary) {
  Summary new_summary(summary);
  return new_summary;
}

}  // end namespace Devcash

#endif /* PRIMITIVES_SUMMARY_H_ */
