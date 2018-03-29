/*
 * transaction.cpp implements the structure of the transaction section of a block.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 */

#include "transaction.h"

#include <stdint.h>

#include "validation.h"
#include "consensus/KeyRing.h"
#include "common/json.hpp"
#include "common/logger.h"
#include "common/ossladapter.h"
#include "oracles/oracleInterface.h"
#include "primitives/smartcoin.h"

using json = nlohmann::json;

namespace Devcash
{

using namespace Devcash;
//static const std::string kSMARTCOIN = "SmartCoin";

struct OpString
{
    std::string jsonVal;
    eOpType flag;
};

const OpString kOpTypes[] =
{
  {"create", eOpType::Create},
  {"modify", eOpType::Modify},
  {"exchange", eOpType::Exchange},
  {"delete", eOpType::Delete}
};

DCTransfer::DCTransfer(std::string& jsonTx, int defaultCoinType)
  : addr_(""), amount_(0), coinIndex_(-1), delay_(0) {
  CASH_TRY {
    size_t pos = 0;
    addr_=jsonFinder(jsonTx, kADDR_TAG, pos);
    amount_=std::stol(jsonFinder(jsonTx, kAMOUNT_TAG, pos));
    std::string coinStr(jsonFinder(jsonTx, kTYPE_TAG, pos));
    if (coinStr.empty()) {
      coinIndex_=defaultCoinType;
    } else {
      coinIndex_=std::stoi(coinStr);
    }
    std::string delayStr(jsonFinder(jsonTx, kDELAY_TAG, pos));
    if (!delayStr.empty()) {
      delay_=std::stol(delayStr);
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "transfer");
  }
}

DCTransfer::DCTransfer() : addr_(""), amount_(0), coinIndex_(-1), delay_(0) {
}

DCTransfer::DCTransfer(std::string& addr, int coinIndex, long amount,
    long delay) : addr_(addr), amount_(amount), coinIndex_(coinIndex),
        delay_(delay) { }

DCTransfer::DCTransfer(std::vector<uint8_t> cbor) {
  CASH_TRY {
    json j = json::from_cbor(cbor);
    addr_ = j[kADDR_TAG].dump();
    amount_ = std::stoi(j[kAMOUNT_TAG].dump());
    coinIndex_ = oracleInterface::getCoinIndexByType(j[kTYPE_TAG]);
    if (!j[kDELAY_TAG].empty()) {
      delay_ = std::stoi(j[kDELAY_TAG].dump());
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "transfer");
  }
}

DCTransfer::DCTransfer(const DCTransfer &other) : addr_(other.addr_),
    amount_(other.amount_), coinIndex_(other.coinIndex_), delay_(other.delay_){
}

std::string DCTransfer::getCanonical()
{
  std::string out("\""+kADDR_TAG+"\":\""+addr_+"\",\""+kTYPE_TAG+"\":"+std::to_string(coinIndex_)+",\""+kAMOUNT_TAG+"\":"
      +std::to_string(amount_));
  if (delay_ > 0) {
    out += ",\""+kDELAY_TAG+"\":"+std::to_string(delay_);
  }
  return out;
}

eOpType mapOpType(std::string oper) {
  if ("0"==oper || "create"==oper) return(eOpType::Create);
  if ("1"==oper || "modify"==oper) return(eOpType::Modify);
  if ("2"==oper || "exchange"==oper) return(eOpType::Exchange);
  if ("3"==oper || "delete"==oper) return(eOpType::Delete);
  CASH_THROW("Invalid operation: "+oper);
}

bool DCTransaction::isOpType(std::string oper) {
  if ("create"==oper) return(oper_ == eOpType::Create);
  if ("modify"==oper) return(oper_ == eOpType::Modify);
  if ("exchange"==oper) return(oper_ == eOpType::Exchange);
  if ("delete"==oper) return(oper_ == eOpType::Delete);
  CASH_THROW("Invalid operation: "+oper);
}

DCTransaction::DCTransaction() : nonce_(0), sig_(""), oper_(eOpType::Create) {
}

DCTransaction::DCTransaction(std::string jsonTx)
  : nonce_(0), sig_(""), oper_(eOpType::Create ){
  CASH_TRY {
    if (jsonTx.at(0) == '{') {
      int coinDex = -1;
      size_t pos = 0;
      std::string opTemp(jsonFinder(jsonTx, kOPER_TAG, pos));
      size_t temp = pos;
      oper_ = mapOpType(opTemp);
      std::string typeStr(jsonFinder(jsonTx, kTYPE_TAG, temp));
      if (!typeStr.empty()) {
        coinDex = oracleInterface::getCoinIndexByType(typeStr);
      }

      size_t dex = jsonTx.find("\""+kXFER_TAG+"\":[", pos);
      dex += kXFER_TAG.size()+4;
      size_t eDex = jsonTx.find("}", dex);
      std::string oneXfer = jsonTx.substr(dex, eDex-dex+1);
      LOG_DEBUG << "One transfer: "+oneXfer;
      DCTransfer* t = new DCTransfer(oneXfer, coinDex);
      xfers_.push_back(*t);
      while (jsonTx.at(eDex+1) != ']' && eDex < jsonTx.size()-1) {
        pos = eDex;
        dex = jsonTx.find("{", pos);
        dex++;
        eDex = jsonTx.find("}", dex);
        oneXfer = jsonTx.substr(dex, eDex-dex+1);
        LOG_DEBUG << "One transfer: "+oneXfer;
        DCTransfer* t = new DCTransfer(oneXfer, coinDex);
        xfers_.push_back(*t);
      }
      nonce_ = std::stoul(jsonFinder(jsonTx, kNONCE_TAG, pos));
      sig_ = jsonFinder(jsonTx, kSIG_TAG, pos);
      LOG_DEBUG << "Transaction signature: "+sig_;
    } else {
      LOG_WARNING << "Invalid transaction input:"+jsonTx+"\n----------------\n";
    }
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

/*DCTransaction::DCTransaction(json jsonObj) {
  CASH_TRY {
    std::string opTemp = jsonObj[kOPER_TAG];
    oper_ = mapOpType(opTemp);
    nonce_ = 0;
    if (!jsonObj[kNONCE_TAG].empty()) nonce_ =
        std::stoi(jsonObj[kNONCE_TAG].dump());
    sig_ = "";
    if (!jsonObj[kSIG_TAG].empty()) {
      sig_ = jsonObj[kSIG_TAG].dump();
      sig_.erase(remove(sig_.begin(), sig_.end(), '\"'), sig_.end());
    }
    for (auto iter = jsonObj[kXFER_TAG].begin(); iter != jsonObj[kXFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer);
      xfers_.push_back(*t);
    }
    hash_ = ComputeHash();
    jsonStr_ = jsonObj.dump();
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}*/

DCTransaction::DCTransaction(std::vector<uint8_t> cbor) {
  CASH_TRY {
    json jsonObj = json::from_cbor(cbor);
    std::string opTemp = jsonObj[kOPER_TAG];
    oper_ = mapOpType(opTemp);
    for (auto iter = jsonObj[kXFER_TAG].begin(); iter != jsonObj[kXFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer, -1);
      xfers_.push_back(*t);
    }
    if (!jsonObj[kNONCE_TAG].empty()) nonce_ =
        std::stoi(jsonObj[kNONCE_TAG].dump());
    sig_ = "";
    if (!jsonObj[kSIG_TAG].empty()) {
      sig_ = jsonObj[kSIG_TAG].dump();
      sig_.erase(remove(sig_.begin(), sig_.end(), '\"'), sig_.end());
    }
    hash_ = ComputeHash();
    jsonStr_ = jsonObj.dump();
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

DCTransaction::DCTransaction(const DCTransaction& tx)
  : xfers_(tx.xfers_)
  , nonce_(tx.nonce_)
  , sig_(tx.sig_)
  , oper_(tx.oper_)
{
}

bool DCTransaction::isValid(DCState& chainState, KeyRing& keys, DCSummary& summary) const {
  CASH_TRY {
    long nValueOut = 0;

    if (nonce_ < 1) {
      LOG_WARNING << "Error: nonce is required\n";
      return(false);
    }

    std::string msg = "\""+kOPER_TAG+"\":\""
        +kOpTypes[oper_].jsonVal+"\",\""+kTYPE_TAG
        +"\":\""+oracleInterface::getCoinTypeByIndex(xfers_[0].coinIndex_)
        +"\",\""+kXFER_TAG+"\":[";
    bool isFirst = true;
    std::string senderAddr;

    for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
      nValueOut += it->amount_;
      if ((oper_ == eOpType::Delete && it->amount_ > 0) ||
        (oper_ != eOpType::Delete && it->amount_ < 0) || oper_ == eOpType::Modify) {
        if (it->delay_ < 0) {
          LOG_WARNING << "Error: A negative delay is not allowed.";
          return false;
        }
      }
        if (!isFirst) {
          msg += ",";
        } else {
          isFirst = false;
        }
        msg += "{\""+kADDR_TAG+"\":\""+it->addr_+"\",\""+kAMOUNT_TAG+"\":"
            +std::to_string(it->amount_);
        if (it->amount_ < 0) {
          if (!senderAddr.empty()) {
            LOG_WARNING << "Multiple senders in transaction!\n";
            return false;
          }
          if ((oper_ == Exchange) && (it->amount_ > chainState.getAmount(it->coinIndex_, it->addr_))) {
            LOG_WARNING << "Coins not available at addr.\n";
            return false;
          }
          senderAddr = it->addr_;
        }
        if (it->delay_ > 0) {
         msg += ",\""+kDELAY_TAG+"\":"+std::to_string(it->delay_);
        }
        msg += "}";
        SmartCoin next_flow(it->coinIndex_, it->addr_, it->amount_);
        chainState.addCoin(next_flow);
        summary.addItem(it->addr_, it->coinIndex_, it->amount_, it->delay_);
    }
    msg += "],\""+kNONCE_TAG+"\":"+std::to_string(nonce_);
    if (nValueOut != 0) {
      LOG_WARNING << "Error: transaction amounts are asymmetric. (sum="+std::to_string(nValueOut)+")";
      return(false);
    }

    if ((oper_ != Exchange) && (!keys.isINN(senderAddr))) {
      LOG_WARNING << "INN transaction not performed by INN!";
      return false;
    }

    EC_KEY* eckey(keys.getKey(senderAddr));

    if (!verifySig(eckey, strHash(msg), sig_)) {
      LOG_WARNING << "Error: transaction signature did not validate.\n";
      LOG_DEBUG << "Validation message is: "+msg;
      LOG_DEBUG << "Sender addr is: "+senderAddr;
      LOG_DEBUG << "Signature is: "+sig_;
      return(false);
    }
    return(true);
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "transaction");
  }
  return(false);
}

long DCTransaction::getValueOut() const
{
  long nValueOut = 0;
  for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
    if (it->amount_ > 0) nValueOut += it->amount_;
  }
  return nValueOut;
}

std::string DCTransaction::getCanonicalForm() {
  std::string out;
  out = "{\""+kOPER_TAG+"\":"+
      std::to_string(static_cast<unsigned int>(oper_))+",\""+
      kXFER_TAG+"\":[";
  bool isFirst = true;
  for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
      if (!isFirst) {
        out += ",";
      }
      isFirst = false;
      out += it->getCanonical();
  }
  out += "],\""+kNONCE_TAG+"\":"+std::to_string(nonce_)+",\""+
  out += kSIG_TAG+"\":\""+sig_+"\"}";
  return out;
}

unsigned int DCTransaction::getByteSize() const
{
  return ToJSON().size();
}

std::string DCTransaction::ComputeHash() const
{
  return strHash(ToJSON());
}

std::string DCTransaction::ToJSON() const
{
  std::string out = "{\""+kOPER_TAG+"\":"
      +std::to_string(static_cast<unsigned int>(oper_))+",\"";
  out+=kXFER_TAG+"\":[";
  json xferArray = json::array();
  bool isFirst = true;
  for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
    if (!isFirst) {
      out += ",";
    }
    isFirst = false;
    out +="{\""+kADDR_TAG+"\":\""+it->addr_+"\",\""+kTYPE_TAG+"\":"+
        std::to_string(it->coinIndex_)+",\""+kAMOUNT_TAG+"\":"+
        std::to_string(it->amount_);
    if (it->delay_ > 0) {
      out += ",\""+kDELAY_TAG+"\":"+std::to_string(it->delay_);
    }
    out+="}";
  }
  out += "],\"";
  out+=kNONCE_TAG+"\":"+std::to_string(nonce_);
  if (sig_.length() > 0) {
    out += ",\""+kSIG_TAG+"\":\""+sig_+"\"";
  }
  out += "}";
  return out;
}

std::vector<uint8_t> DCTransaction::ToCBOR() const
{
  return(json::to_cbor(ToJSON()));
}

} //end namespace Devcash
