/*
 * validation.cpp implements the structure of the validation section of a block.
 *
 *  Created on: Jan 3, 2018
 *  Author: Nick Williams
 */

#include "validation.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>

#include "common/json.hpp"
#include "common/logger.h"
#include "common/ossladapter.h"

namespace Devcash
{

using json = nlohmann::json;

DCSummary::DCSummary() {
}

std::pair<long, DCSummaryItem> getSummaryPair(std::string raw) {
  int dex = raw.find(',');
  long coinType = stol(raw.substr(0, dex));
  int sdex = raw.find(',', dex+1);
  long delta = 0;
  long delay = 0;
  if (sdex < 0) {
    delta = stol(raw.substr(dex));
  } else {
    delta = stol(raw.substr(dex, sdex));
    delay = stol(raw.substr(sdex));
  }
  DCSummaryItem oneSum(delta, delay);
  std::pair<long, DCSummaryItem> out(coinType, oneSum);
  return(out);
}

coinmap getAddrSummary(std::string raw) {
  std::unordered_map<long, DCSummaryItem> out;
  std::string temp;
  std::stringstream ss(raw);
  while (std::getline(ss, temp, ')')) {
    out.insert(getSummaryPair(temp));
    std::getline(ss, temp, '(');
  }
  return(out);
}

DCSummary::DCSummary(std::string canonical) {
  //CASH_TRY {
    if (canonical.at(0) == '{') {
      std::stringstream ss(canonical);
      std::string temp = "";
      std::string addr = "";
      std::string coinVector = "";

      std::getline(ss, temp, '{');
      while (std::getline(ss, addr, ':')) {
        std::getline(ss, temp, '[');
        std::getline(ss, coinVector, ']');
        std::pair<std::string, coinmap> addrSet(addr,
            getAddrSummary(coinVector));
        summary_.insert(addrSet);
      }
    }
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "summary");
  }*/
}

DCSummary::DCSummary(std::unordered_map<std::string,
    std::unordered_map<long, DCSummaryItem>> summary) :
    summary_(summary) {
}

bool DCSummary::addItem(std::string addr, long coinType, DCSummaryItem item) {
  //CASH_TRY {
    if (summary_.count(addr) > 0) {
      std::unordered_map<long, DCSummaryItem> existing(summary_.at(addr));
      if (existing.count(coinType) > 0) {
        DCSummaryItem theItem = existing.at(coinType);
        theItem.delta += item.delta;
        theItem.delay = std::max(theItem.delay, item.delay);
        std::pair<long, DCSummaryItem> oneItem(coinType, theItem);
        existing.insert(oneItem);
      } else {
        std::pair<long, DCSummaryItem> oneItem(coinType, item);
        existing.insert(oneItem);
      }
      std::pair<std::string,
        std::unordered_map<long, DCSummaryItem>> oneSummary(addr, existing);
      summary_.insert(oneSummary);
    } else {
      std::pair<long, DCSummaryItem> newPair(coinType, item);
      std::unordered_map<long, DCSummaryItem> newSummary;
      newSummary.insert(newPair);
      std::pair<std::string,
        std::unordered_map<long, DCSummaryItem>> oneSummary(addr, newSummary);
      summary_.insert(oneSummary);
    }
    return true;
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "summary");
    return false;
  }*/
}

bool DCSummary::addItem(std::string addr, long coinType, long delta,
    long delay) {
  DCSummaryItem item(delta, delay);
  return addItem(addr, coinType, item);
}

bool DCSummary::addItems(std::string addr,
    std::unordered_map<long, DCSummaryItem> items) {
  //CASH_TRY {
    if (summary_.count(addr) > 0) {
      std::unordered_map<long, DCSummaryItem> existing(summary_.at(addr));
      for (auto iter = items.begin(); iter != items.end(); ++iter) {
        if (existing.count(iter->first) > 0) {
          DCSummaryItem theItem = existing.at(iter->first);
          theItem.delta += iter->second.delta;
          theItem.delay = std::max(theItem.delay, iter->second.delay);
          std::pair<long, DCSummaryItem> oneItem(iter->first, theItem);
          existing.insert(oneItem);
        } else {
          std::pair<long, DCSummaryItem> oneItem(iter->first, iter->second);
          existing.insert(oneItem);
        }
      }
      std::pair<std::string,
        std::unordered_map<long, DCSummaryItem>> updateSummary(addr, existing);
      summary_.insert(updateSummary);
    } else {
      std::pair<std::string,
        std::unordered_map<long, DCSummaryItem>> oneSummary(addr, items);
      summary_.insert(oneSummary);
    }
    return true;
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "summary");
    return false;
  }*/
}

std::string DCSummary::toCanonical() {
  std::string out = "{";
  for (auto iter = summary_.begin(); iter != summary_.end(); ++iter) {
    std::string addr(iter->first);
    std::unordered_map<long, DCSummaryItem> coinTypes(iter->second);
    if (!coinTypes.empty()) {
      out += addr+":[";
      bool isFirst = true;
      for (auto j = coinTypes.begin(); j != coinTypes.end(); ++j) {
        if (!isFirst) out += ",";
        out += "("+std::to_string(j->first);
        out += ","+std::to_string(j->second.delta);
        if (j->second.delay > 0) out += ","+std::to_string(j->second.delay);
        out += ")";
      }
      out += "]";
    }
  }
  out += "}";
  return out;
}

/* All transactions must be symmetric wrt coin type */
bool DCSummary::isSane() {
  //CASH_TRY {
    if (summary_.empty()) return false;
    long coinTotal = 0;
    for (auto iter = summary_.begin(); iter != summary_.end(); ++iter) {
      coinTotal = 0;
      for (auto j = iter->second.begin(); j != iter->second.end(); ++j) {
        coinTotal += j->second.delta;
      }
      if (coinTotal != 0) return false;
    }
    return true;
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "summary");
    return false;
  }*/
}

DCValidationBlock::DCValidationBlock(std::string jsonMsg) {
  //CASH_TRY {
    json j = json::parse(jsonMsg);
    json temp = json::array();
    for (auto iter = j.begin(); iter != j.end(); ++iter) {
      std::string key(iter.key());
      std::string value = iter.value();
      std::pair<std::string, std::string> oneSig(iter.key(), value);
      sigs_.insert(oneSig);
      temp += json::object({iter.key(), iter.value()});
    }
    hash_ = ComputeHash();
    jsonStr_ = temp.dump();
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }*/
}

DCValidationBlock::DCValidationBlock() {
}

DCValidationBlock::DCValidationBlock(std::vector<uint8_t> cbor) {
  //CASH_TRY {
    json j = json::from_cbor(cbor);
    json temp = json::array();
    for (auto iter = j.begin(); iter != j.end(); ++iter) {
      std::string key(iter.key());
      std::string value = iter.value();
      std::pair<std::string, std::string> oneSig(iter.key(), value);
      sigs_.insert(oneSig);
      temp += json::object({iter.key(), iter.value()});
    }
    hash_ = ComputeHash();
    jsonStr_ = temp.dump();
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }*/
}

DCValidationBlock::DCValidationBlock(const DCValidationBlock& other)
  : sigs_(other.sigs_), summaryObj_(other.summaryObj_) {
}

DCValidationBlock::DCValidationBlock(std::string node, std::string sig) {
  //CASH_TRY {
    std::pair<std::string, std::string> oneSig(node, sig);
    sigs_.insert(oneSig);
    hash_ = ComputeHash();
    json j = {node, sig};
    jsonStr_ = j.dump();
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }*/
}

DCValidationBlock::DCValidationBlock(vmap sigMap) {
  //CASH_TRY {
    //sigs(sigMap);
    hash_ = ComputeHash();
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }*/
}

bool DCValidationBlock::addValidation(std::string node, std::string sig) {
  //CASH_TRY {
    std::pair<std::string, std::string> oneSig(node, sig);
    sigs_.insert(oneSig);
    hash_ = ComputeHash();
    return(true);
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }*/
  return(false);
}

unsigned int DCValidationBlock::GetByteSize() const
{
  return ToCBOR().size();
}

unsigned int DCValidationBlock::GetValidationCount() const
{
  return sigs_.size();
}

std::string DCValidationBlock::ComputeHash() const
{
  return strHash(ToJSON());
}

std::string DCValidationBlock::ToJSON() const
{
  json j = {};
  for (auto it : sigs_) {
    json thisSig = {it.first, it.second};
    j += thisSig;
  }
  return(j.dump());
}

std::vector<uint8_t> DCValidationBlock::ToCBOR() const
{
  return(json::to_cbor(ToJSON()));
}

} //end namespace Devcash
