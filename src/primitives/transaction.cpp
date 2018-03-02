/*
 * transaction.cpp implements the structure of the transaction section of a block.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 */

#include "transaction.h"

#include <stdint.h>

#include "../common/json.hpp"
#include "../common/logger.h"
#include "../common/ossladapter.h"
#include "../common/util.h"

using json = nlohmann::json;

namespace Devcash
{

using namespace Devcash;

//exception toggling capability
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && not defined(DEVCASH_NOEXCEPTION)
    #define CASH_THROW(exception) throw exception
    #define CASH_TRY try
    #define CASH_CATCH(exception) catch(exception)
#else
    #define CASH_THROW(exception) std::abort()
    #define CASH_TRY if(true)
    #define CASH_CATCH(exception) if(false)
#endif

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
    nonce_ = 0;
    if (!j[kNONCE_TAG].empty()) nonce_ = std::stoi(j[kNONCE_TAG].dump());
    sig_ = "";
    if (!j[kSIG_TAG].empty()) {
      sig_ = j[kSIG_TAG].dump();
      sig_.erase(remove(sig_.begin(), sig_.end(), '\"'), sig_.end());
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "transaction");
  }
}

DCTransfer::DCTransfer() {
  addr_ = "";
  amount_ = 0;
  nonce_ = 0;
  sig_ = "";
}

DCTransfer::DCTransfer(std::vector<uint8_t> cbor) {
  CASH_TRY {
    json j = json::from_cbor(cbor);
    addr_ = j[kADDR_TAG].dump();
    amount_ = std::stoi(j[kAMOUNT_TAG].dump());
    nonce_ = 0;
    if (!j[kNONCE_TAG].empty()) nonce_ = std::stoi(j[kNONCE_TAG].dump());
    sig_ = "";
    if (!j[kSIG_TAG].empty()) {
      sig_ = j[kSIG_TAG].dump();
      sig_.erase(remove(sig_.begin(), sig_.end(), '\"'), sig_.end());
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "transaction");
  }
}

DCTransfer::DCTransfer(const DCTransfer &other) {
  if (this != &other) {
    this->addr_ = other.addr_;
    this->amount_ = other.amount_;
    this->nonce_ = other.nonce_;
    this->sig_ = other.sig_;
  }
}

std::string DCTransfer::getCanonical()
{
  std::string out("\""+kADDR_TAG+"\":\""+addr_+"\",\""+kAMOUNT_TAG+"\":"
      +std::to_string(amount_)+",\""+kNONCE_TAG+"\":"+std::to_string(nonce_));
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

DCTransaction::DCTransaction() {
  xfers_ = nullptr;
  oper_ = eOpType::Create;
  type_ = "SmartCoin";
  delay_ = 0;
}

DCTransaction::DCTransaction(std::string jsonTx) {
  oper_ = eOpType::Create;
  type_ = kSMARTCOIN;
  xfers_ = nullptr;
  delay_ = 0;
  CASH_TRY {
    json jsonObj = json::parse(jsonTx);
    std::string opTemp = jsonObj[kOPER_TAG];
    oper_ = mapOpType(opTemp);
    type_ = jsonObj[kTYPE_TAG];
    xfers_ = new std::vector<DCTransfer>(0);
    for (auto iter = jsonObj[kXFER_TAG].begin(); iter != jsonObj[kXFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer);
      xfers_->push_back(*t);
    }
    hash_ = ComputeHash();
    jsonStr_ = jsonObj.dump();
    if (!jsonObj[kDELAY_TAG].empty()) {
      delay_ = std::stoi(jsonObj[kDELAY_TAG].dump());
    }
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

DCTransaction::DCTransaction(json jsonObj) {
  oper_ = eOpType::Create;
  type_ = kSMARTCOIN;
  xfers_ = nullptr;
  delay_ = 0;
  CASH_TRY {
    std::string opTemp = jsonObj[kOPER_TAG];
    oper_ = mapOpType(opTemp);
    type_ = jsonObj[kTYPE_TAG];
    xfers_ = new std::vector<DCTransfer>(0);
    for (auto iter = jsonObj[kXFER_TAG].begin(); iter != jsonObj[kXFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer);
      xfers_->push_back(*t);
    }
    hash_ = ComputeHash();
    jsonStr_ = jsonObj.dump();
    if (!jsonObj[kDELAY_TAG].empty()) {
      delay_ = std::stoi(jsonObj[kDELAY_TAG].dump());
    }
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

DCTransaction::DCTransaction(std::vector<uint8_t> cbor) {
  oper_ = eOpType::Create;
  type_ = kSMARTCOIN;
  xfers_ = nullptr;
  delay_ = 0;
  CASH_TRY {
    json jsonObj = json::from_cbor(cbor);
    std::string opTemp = jsonObj[kOPER_TAG];
    oper_ = mapOpType(opTemp);
    type_ = jsonObj[kTYPE_TAG];
    xfers_ = new std::vector<DCTransfer>(0);
    for (auto iter = jsonObj[kXFER_TAG].begin(); iter != jsonObj[kXFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer);
      xfers_->push_back(*t);
    }
    hash_ = ComputeHash();
    jsonStr_ = jsonObj.dump();
    if (!jsonObj[kDELAY_TAG].empty()) {
      delay_ = std::stoi(jsonObj[kDELAY_TAG].dump());
    }
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

DCTransaction::DCTransaction(const DCTransaction& tx) : xfers_(tx.xfers_) {
  oper_ = tx.oper_;
  type_ = tx.type_;
  hash_ = tx.hash_;
  delay_ = tx.delay_;
}

bool DCTransaction::isValid(EC_KEY* eckey) const {
  CASH_TRY {
    long nValueOut = 0;
    if (delay_ < 0) {
      LOG_WARNING << "Error: A negative delay is not allowed.";
      return false;
    }
    for (std::vector<DCTransfer>::iterator it = xfers_->begin(); it != xfers_->end(); ++it) {
      nValueOut += it->amount_;
      if ((oper_ == eOpType::Delete && it->amount_ > 0) ||
        (oper_ != eOpType::Delete && it->amount_ < 0) || oper_ == eOpType::Modify) {
        if (it->nonce_ < 1) {
          LOG_WARNING << "Error: nonce is required for outgoing tx\n";
          return(false);
        }
        std::string msg("\"addr\":"+it->addr_+",\"amount\":"
          +std::to_string(it->amount_)
          +",\"nonce\":"+std::to_string(it->nonce_));
        //TODO: use hash instead of raw msg, strHash(msg)
        if (!verifySig(eckey, msg, it->sig_)) {
          LOG_WARNING << "Error: signature did not validate.\n";
          return(false);
        }
      }
    }
    if (nValueOut != 0) {
      LOG_WARNING << "Error: transaction amounts are asymmetric.\n";
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
  for (std::vector<DCTransfer>::iterator it = xfers_->begin();
      it != xfers_->end(); ++it) {
    if (it->amount_ > 0) nValueOut += it->amount_;
  }
  return nValueOut;
}

unsigned int DCTransaction::getByteSize() const
{
  return jsonStr_.size();
}

std::string DCTransaction::ComputeHash() const
{
  return strHash(ToJSON());
}

std::string DCTransaction::ToJSON() const
{
  json j = {{kOPER_TAG, oper_},{kTYPE_TAG, type_}};
  json xferArray = json::array();
  for (std::vector<DCTransfer>::iterator it = xfers_->begin();
    it != xfers_->end(); ++it) {
      json thisXfer = {{"addr", it->addr_}, {"amount", it->amount_}};
      if (it->nonce_ > 0) thisXfer += {"nonce", it->nonce_};
      if (it->sig_.length() > 0) thisXfer += {"sig", it->sig_};
      xferArray += thisXfer;
  }
  j += {kXFER_TAG, xferArray};
  return(j.dump());
}

std::vector<uint8_t> DCTransaction::ToCBOR() const
{
  return(json::to_cbor(ToJSON()));
}

} //end namespace Devcash
