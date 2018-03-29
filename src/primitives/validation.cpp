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
#include <vector>

#include "transaction.h"
#include "common/json.hpp"
#include "common/logger.h"
#include "common/ossladapter.h"

namespace Devcash
{

using json = nlohmann::json;

DCSummary::DCSummary() {
}

std::pair<long, DCSummaryItem> getSummaryPair(std::string raw) {
  if (raw.front() == '(') raw.erase(std::begin(raw));
  int dex = raw.find(',');
  long coinType = stol(raw.substr(0, dex));
  int sdex = raw.find(',', dex+1);
  long delta = 0;
  long delay = 0;
  std::string dStr;
  std::string delayStr = "";
  if (sdex < 0) {
    delta = stol(raw.substr(dex+1));
  } else {
    delta = stol(raw.substr(dex+1, sdex));
    delay = stol(raw.substr(sdex+1));
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
  CASH_TRY {
    LOG_DEBUG << "CanonicalSummary: "+canonical;
    if (canonical.at(0) == '{') {
      std::stringstream ss(canonical);

      size_t dex = 0;
      size_t eDex = 0;
      while (eDex < canonical.size()-1) {
        dex = canonical.find("\"", dex);
        dex++;
        eDex = canonical.find("\":[(", dex);
        if (eDex == std::string::npos) return;
        std::string oneAddr = canonical.substr(dex, eDex-dex);
        std::string points;
        if (!oneAddr.empty()) {
          dex = eDex+3;
          eDex = canonical.find("],\"", dex);
          points = canonical.substr(dex, eDex-dex);
          LOG_DEBUG << "Insert summary addr: "+oneAddr;
          std::pair<std::string, coinmap> addrSet(oneAddr,
            getAddrSummary(points));
          summary_.insert(addrSet);
        }
      }
    }
    LOG_DEBUG << "Summary state: "+toCanonical();
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "summary");
  }
}

DCSummary::DCSummary(std::map<std::string,
    std::unordered_map<long, DCSummaryItem>> summary) :
    summary_(summary) {
}

DCSummary::DCSummary(const DCSummary& other)
  : summary_(other.summary_) {
}

bool DCSummary::addItem(std::string addr, long coinType, DCSummaryItem item) {
  CASH_TRY {
    std::lock_guard<std::mutex> lock(lock_);
    if (summary_.count(addr) > 0) {
      std::unordered_map<long, DCSummaryItem> existing(summary_.at(addr));
      if (existing.count(coinType) > 0) {
        DCSummaryItem the_item = existing.at(coinType);
        the_item.delta += item.delta;
        the_item.delay = std::max(the_item.delay, item.delay);
        std::pair<long, DCSummaryItem> one_item(coinType, the_item);
        existing.insert(one_item);
      } else {
        std::pair<long, DCSummaryItem> one_item(coinType, item);
        existing.insert(one_item);
      }
      std::pair<std::string,
        std::unordered_map<long, DCSummaryItem>> one_summary(addr, existing);
      summary_.insert(one_summary);
    } else {
      std::pair<long, DCSummaryItem> new_pair(coinType, item);
      std::unordered_map<long, DCSummaryItem> new_summary;
      new_summary.insert(new_pair);
      std::pair<std::string,
        std::unordered_map<long, DCSummaryItem>> one_summary(addr, new_summary);
      summary_.insert(one_summary);
    }
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "summary");
    return false;
  }
}

bool DCSummary::addItem(std::string addr, long coinType, long delta,
    long delay) {
  DCSummaryItem item(delta, delay);
  return addItem(addr, coinType, item);
}

bool DCSummary::addItems(std::string addr,
  std::unordered_map<long, DCSummaryItem> items) {
  CASH_TRY {
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
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "summary");
    return false;
  }
}

std::string DCSummary::toCanonical() {
  std::string out = "{";
  bool first_addr = true;
  for (auto iter = summary_.begin(); iter != summary_.end(); ++iter) {
    std::string addr(iter->first);
    std::unordered_map<long, DCSummaryItem> coinTypes(iter->second);
    if (!coinTypes.empty()) {
      if (!first_addr) out += ",";
      first_addr = false;
      out += "\""+addr+"\":[";
      bool isFirst = true;
      for (auto j = coinTypes.begin(); j != coinTypes.end(); ++j) {
        if (!isFirst) out += ",";
        isFirst = false;
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

size_t DCSummary::getByteSize() {
  return toCanonical().size();
}

/* All transactions must be symmetric wrt coin type */
bool DCSummary::isSane() {
  CASH_TRY {
    if (summary_.empty()) return false;
    long coinTotal = 0;
    for (auto iter = summary_.begin(); iter != summary_.end(); ++iter) {
      for (auto j = iter->second.begin(); j != iter->second.end(); ++j) {
        coinTotal += j->second.delta;
      }
    }
    if (coinTotal != 0) {
      LOG_DEBUG << "Summary state invalid: "+toCanonical();
      return false;
    }
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "summary");
    return false;
  }
}

DCValidationBlock::DCValidationBlock(std::string& jsonMsg) {
  CASH_TRY {
    size_t pos = 0;
    /*size_t dex = jsonMsg.find("\"sum\":", pos);
    if (dex == std::string::npos) {
      LOG_WARNING << "Invalid validation section!";
      LOG_WARNING << "Invalid validation section: "+jsonMsg;
      return;
    }
    dex += 6;
    size_t eDex = jsonMsg.find("]},\"vals", dex);
    if (eDex == std::string::npos) {
      LOG_WARNING << "Invalid validation section!";
      LOG_WARNING << "Invalid validation section: "+jsonMsg;
      return;
    }
    pos = eDex;
    std::string sumTemp = jsonMsg.substr(dex, eDex-dex+2);
    summaryObj_ = DCSummary(sumTemp);

    if (!summaryObj_.isSane()) {
      LOG_WARNING << "Summary is invalid!\n";
      LOG_DEBUG << "Summary state: "+summaryObj_.toCanonical();
    }*/


    size_t dex = jsonMsg.find("\"vals\":[", pos);
    dex += 8;
    size_t eDex = jsonMsg.find("\":\"", dex);
    std::string oneAddr = jsonMsg.substr(dex+1, eDex-dex-1);
    std::string oneSig;
    if (!oneAddr.empty()) {
      LOG_DEBUG << "Signature addr: "+oneAddr;
      dex = eDex+3;
      eDex = jsonMsg.find("\",\"", dex);
      if (eDex == std::string::npos) {
        eDex = jsonMsg.find("\"]", dex);
        oneSig = jsonMsg.substr(dex, eDex-dex);
        LOG_DEBUG << "Final Signature: "+oneSig;
        std::pair<std::string, std::string> onePair(oneAddr, oneSig);
        sigs_.insert(onePair);
        return;
      }

      oneSig = jsonMsg.substr(dex, eDex-dex-3);
      LOG_DEBUG << "Signature: "+oneSig;
      std::pair<std::string, std::string> onePair(oneAddr, oneSig);
      sigs_.insert(onePair);

      while (eDex < jsonMsg.size()) {
        dex = eDex+3;
        eDex = jsonMsg.find("\":\"", dex);
        oneAddr = jsonMsg.substr(dex, eDex-dex-3);
        LOG_DEBUG << "Signature addr: "+oneAddr;
        dex = eDex+3;
        eDex = jsonMsg.find("\",\"", dex);
        if (eDex == std::string::npos) {
          eDex = jsonMsg.find("\"]", dex);
          oneSig = jsonMsg.substr(dex, eDex-dex-2);
          LOG_DEBUG << "Final Signature: "+oneSig;
          std::pair<std::string, std::string> onePair(oneAddr, oneSig);
          sigs_.insert(onePair);
          return;
        }
        oneSig = jsonMsg.substr(dex, eDex-dex-3);
        LOG_DEBUG << "Signature: "+oneSig;
        std::pair<std::string, std::string> onePair(oneAddr, oneSig);
        sigs_.insert(onePair);
      }
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }
}

DCValidationBlock::DCValidationBlock() {
}

DCValidationBlock::DCValidationBlock(std::vector<uint8_t> cbor) {
  CASH_TRY {
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
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }
}

DCValidationBlock::DCValidationBlock(const DCValidationBlock& other)
  : sigs_(other.sigs_)
  , summaryObj_(other.summaryObj_) {
}

DCValidationBlock::DCValidationBlock(std::string node, std::string sig) {
  CASH_TRY {
    std::pair<std::string, std::string> oneSig(node, sig);
    sigs_.insert(oneSig);
    hash_ = ComputeHash();
    json j = {node, sig};
    jsonStr_ = j.dump();
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }
}

DCValidationBlock::DCValidationBlock(vmap sigMap) : sigs_(sigMap) {
  CASH_TRY {
    hash_ = ComputeHash();
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }
}

bool DCValidationBlock::addValidation(std::string node, std::string sig) {
  CASH_TRY {
    std::pair<std::string, std::string> oneSig(node, sig);
    sigs_.insert(oneSig);
    hash_ = ComputeHash();
    return(true);
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }
  return(false);
}

bool DCValidationBlock::addValidation(DCValidationBlock& other) {
  CASH_TRY {
    for (auto iter = other.sigs_.begin(); iter != other.sigs_.end(); ++iter) {
      if (sigs_.count(iter->first) == 0) {
        sigs_.insert(std::pair<std::string, std::string>(iter->first, iter->second));
      }
    }
    return(true);
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "validation");
  }
  return(false);
}

/*std::string DCValidationBlock::CalculateSummary(const std::vector<DCTransaction>& txs) {
  for (auto it : txs) {
    for (auto j : it.xfers_) {
      summaryObj_.addItem(j.addr_, j.coinIndex_,j.amount_,j.delay_);
    }
  }
  return summaryObj_.toCanonical();
}*/

unsigned int DCValidationBlock::GetByteSize() const
{
  return ToJSON().size();
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
  std::string out = "\"sum\":";
  out += "{";
  bool first_addr = true;
  for (auto iter = summaryObj_.summary_.begin();
      iter != summaryObj_.summary_.end(); ++iter) {
    std::string addr(iter->first);
    std::unordered_map<long, DCSummaryItem> coinTypes(iter->second);
    if (!coinTypes.empty()) {
      if (!first_addr) out += ",";
      first_addr = false;
      out += "\""+addr+"\":[";
      bool isFirst = true;
      for (auto j = coinTypes.begin(); j != coinTypes.end(); ++j) {
        if (!isFirst) out += ",";
        isFirst = false;
        out += "("+std::to_string(j->first);
        out += ","+std::to_string(j->second.delta);
        if (j->second.delay > 0) out += ","+std::to_string(j->second.delay);
        out += ")";
      }
      out += "]";
    }
  }
  out += "}";
  out += ",\"vals\":[";
  bool isFirst = true;
  for (auto it : sigs_) {
    if (!isFirst) {
      out += ",";
    }
    isFirst = false;
    out += "\""+it.first+"\":\""+it.second+"\"";
  }
  out += "]";
  return(out);
}

std::vector<uint8_t> DCValidationBlock::ToCBOR() const
{
  return(json::to_cbor(ToJSON()));
}

} //end namespace Devcash
