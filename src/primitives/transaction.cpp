/*
 * transaction.cpp implements the structure of the transaction section of a block.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 */

#include "transaction.h"

#include <stdint.h>

#include "common/json.hpp"
#include "common/logger.h"
#include "common/ossladapter.h"
#include "oracles/oracleInterface.h"

using json = nlohmann::json;

namespace Devcash
{

using namespace Devcash;

static const std::string kOPER_TAG = "oper";
static const std::string kTYPE_TAG = "type";
static const std::string kXFER_TAG = "xfer";
static const std::string kDELAY_TAG = "dlay";
static const std::string kADDR_TAG = "addr";
static const std::string kAMOUNT_TAG = "amount";
static const std::string kNONCE_TAG = "nonce";
static const std::string kSIG_TAG = "sig";
static const std::string kSMARTCOIN = "SmartCoin";

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

DCTransfer::DCTransfer(std::string jsonTx) {
  CASH_TRY {
    json j = json::parse(jsonTx);
    addr_ = j[kADDR_TAG].dump();
    amount_ = std::stoi(j[kAMOUNT_TAG].dump());
    coinIndex_ = oracleInterface::getCoinIndexByType(j[kTYPE_TAG]);
    if (!j[kDELAY_TAG].empty()) {
      delay_ = std::stoi(j[kDELAY_TAG].dump());
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "transaction");
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
    LOG_WARNING << FormatException(&e, "transaction");
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
  return (out);
}

eOpType mapOpType(std::string oper) {
  if ("create"==oper) return(eOpType::Create);
  if ("modify"==oper) return(eOpType::Modify);
  if ("exchange"==oper) return(eOpType::Exchange);
  if ("delete"==oper) return(eOpType::Delete);
  CASH_THROW("Invalid operation: "+oper);
}

bool DCTransaction::isOpType(std::string oper) {
  if ("create"==oper) return(oper_ == eOpType::Create);
  if ("modify"==oper) return(oper_ == eOpType::Modify);
  if ("exchange"==oper) return(oper_ == eOpType::Exchange);
  if ("delete"==oper) return(oper_ == eOpType::Delete);
  CASH_THROW("Invalid operation: "+oper);
}

DCTransaction::DCTransaction() : oper_(eOpType::Create), nonce_(0), sig_("") {
}

DCTransaction::DCTransaction(std::string jsonTx) {
  CASH_TRY {
    json jsonObj = json::parse(jsonTx);
    std::string opTemp = jsonObj[kOPER_TAG];
    oper_ = mapOpType(opTemp);
    for (auto iter = jsonObj[kXFER_TAG].begin();
        iter != jsonObj[kXFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer);
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

DCTransaction::DCTransaction(json jsonObj) {
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
}

DCTransaction::DCTransaction(std::vector<uint8_t> cbor) {
  CASH_TRY {
    json jsonObj = json::from_cbor(cbor);
    std::string opTemp = jsonObj[kOPER_TAG];
    oper_ = mapOpType(opTemp);
    for (auto iter = jsonObj[kXFER_TAG].begin(); iter != jsonObj[kXFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer);
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

DCTransaction::DCTransaction(const DCTransaction& tx) : xfers_(tx.xfers_),
    oper_(tx.oper_), nonce_(tx.nonce_), sig_(tx.sig_) {
}

bool DCTransaction::isValid(EC_KEY* eckey) const {
  CASH_TRY {
    long nValueOut = 0;

    if (nonce_ < 1) {
      LOG_WARNING << "Error: nonce is required\n";
      return(false);
    }
    for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
      nValueOut += it->amount_;
      if ((oper_ == eOpType::Delete && it->amount_ > 0) ||
        (oper_ != eOpType::Delete && it->amount_ < 0) || oper_ == eOpType::Modify) {
        if (it->delay_ < 0) {
          LOG_WARNING << "Error: A negative delay is not allowed.";
          return false;
        }
      }
    }
    if (nValueOut != 0) {
      LOG_WARNING << "Error: transaction amounts are asymmetric.\n";
      return(false);
    }
    std::string msg(ToJSON());
    //TODO: use hash instead of raw msg, strHash(msg)
    if (!verifySig(eckey, msg, sig_)) {
      LOG_WARNING << "Error: signature did not validate.\n";
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

std::string const DCTransaction::getCanonical() {
  std::string out("{\""+kOPER_TAG+"\":"+
      std::to_string(static_cast<unsigned int>(oper_))+",\""+
      kNONCE_TAG+"\":"+std::to_string(nonce_)+",\""+kXFER_TAG+"\":[");
  bool isFirst = true;
  for (std::vector<DCTransfer>::iterator it = xfers_.begin();
    it != xfers_.end(); ++it) {
      if (!isFirst) {
        out += ",";
      } else {
        isFirst = false;
      }
      out += it->getCanonical();
  }
  out += "],\""+kSIG_TAG+"\":"+sig_+"}";
  return (out);
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
  json j = {{kOPER_TAG, oper_},{kNONCE_TAG, nonce_}};
  json xferArray = json::array();
  for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
      json thisXfer = {{kADDR_TAG, it->addr_}, {kTYPE_TAG, it->coinIndex_},
          {kAMOUNT_TAG, it->amount_}};
      if (it->delay_ > 0) thisXfer += {kDELAY_TAG, it->delay_};
      xferArray += thisXfer;
  }
  j += {kXFER_TAG, xferArray};
  if (sig_.length() > 0) j += {kSIG_TAG, sig_};
  return(j.dump());
}

std::vector<uint8_t> DCTransaction::ToCBOR() const
{
  return(json::to_cbor(ToJSON()));
}

} //end namespace Devcash
