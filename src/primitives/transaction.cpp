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

static const std::string OPER_TAG = "oper";
static const std::string TYPE_TAG = "type";
static const std::string XFER_TAG = "xfer";
static const std::string SMARTCOIN = "SmartCoin";

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
    addr_ = j["addr"].dump();
    amount_ = std::stoi(j["amount"].dump());
    nonce_ = 0;
    if (!j["nonce"].empty()) nonce_ = std::stoi(j["nonce"].dump());
    sig_ = "";
    if (!j["sig"].empty()) {
      sig_ = j["sig"].dump();
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
    addr_ = j["addr"].dump();
    amount_ = std::stoi(j["amount"].dump());
    nonce_ = 0;
    if (!j["nonce"].empty()) nonce_ = std::stoi(j["nonce"].dump());
    sig_ = "";
    if (!j["sig"].empty()) {
      sig_ = j["sig"].dump();
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
  std::string out("\"addr\":\""+addr_+"\",\"amount\":"
      +std::to_string(amount_)+",\"nonce\":"+std::to_string(nonce_));
  return (out);
}

eOpType getOpType(std::string oper) {
  if ("create"==oper) return(eOpType::Create);
  if ("modify"==oper) return(eOpType::Modify);
  if ("exchange"==oper) return(eOpType::Exchange);
  if ("delete"==oper) return(eOpType::Delete);
  CASH_THROW("Invalid operation: "+oper);
}

DCTransaction::DCTransaction() {
  xfers_ = nullptr;
  oper_ = eOpType::Create;
  type_ = "SmartCoin";
}

DCTransaction::DCTransaction(std::string jsonTx) {
  oper_ = eOpType::Create;
  type_ = SMARTCOIN;
  xfers_ = nullptr;
  CASH_TRY {
    json jsonObj = json::parse(jsonTx);
    std::string opTemp = jsonObj[OPER_TAG];
    oper_ = getOpType(opTemp);
    type_ = jsonObj[TYPE_TAG];
    xfers_ = new std::vector<DCTransfer>(0);
    for (auto iter = jsonObj[XFER_TAG].begin(); iter != jsonObj[XFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer);
      xfers_->push_back(*t);
    }
    hash_ = ComputeHash();
    jsonStr_ = jsonObj.dump();
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

DCTransaction::DCTransaction(json jsonObj) {
  oper_ = eOpType::Create;
  type_ = SMARTCOIN;
  xfers_ = nullptr;
  CASH_TRY {
    std::string opTemp = jsonObj[OPER_TAG];
    oper_ = getOpType(opTemp);
    type_ = jsonObj[TYPE_TAG];
    xfers_ = new std::vector<DCTransfer>(0);
    for (auto iter = jsonObj[XFER_TAG].begin(); iter != jsonObj[XFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer);
      xfers_->push_back(*t);
    }
    hash_ = ComputeHash();
    jsonStr_ = jsonObj.dump();
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

DCTransaction::DCTransaction(std::vector<uint8_t> cbor) {
  oper_ = eOpType::Create;
  type_ = SMARTCOIN;
  xfers_ = nullptr;
  CASH_TRY {
    json jsonObj = json::from_cbor(cbor);
    std::string opTemp = jsonObj[OPER_TAG];
    oper_ = getOpType(opTemp);
    type_ = jsonObj[TYPE_TAG];
    xfers_ = new std::vector<DCTransfer>(0);
    for (auto iter = jsonObj[XFER_TAG].begin(); iter != jsonObj[XFER_TAG].end(); ++iter) {
      std::string xfer = iter.value().dump();
      DCTransfer* t = new DCTransfer(xfer);
      xfers_->push_back(*t);
    }
    hash_ = ComputeHash();
    jsonStr_ = jsonObj.dump();
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

DCTransaction::DCTransaction(const DCTransaction& tx) : xfers_(tx.xfers_) {
  oper_ = tx.oper_;
  type_ = tx.type_;
  hash_ = tx.hash_;
}

bool DCTransaction::isValid(EC_KEY* eckey) const {
  CASH_TRY {
    long nValueOut = 0;
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

long DCTransaction::GetValueOut() const
{
  long nValueOut = 0;
  for (std::vector<DCTransfer>::iterator it = xfers_->begin(); it != xfers_->end(); ++it) {
    nValueOut += it->amount_;
  }
  return nValueOut;
}

unsigned int DCTransaction::GetByteSize() const
{
  return jsonStr_.size();
}

std::string DCTransaction::ComputeHash() const
{
  return strHash(ToJSON());
}

std::string DCTransaction::ToJSON() const
{
  json j = {{OPER_TAG, oper_},{TYPE_TAG, type_}};
  json xferArray = json::array();
  for (std::vector<DCTransfer>::iterator it = xfers_->begin();
    it != xfers_->end(); ++it) {
      json thisXfer = {{"addr", it->addr_}, {"amount", it->amount_}};
      if (it->nonce_ > 0) thisXfer += {"nonce", it->nonce_};
      if (it->sig_.length() > 0) thisXfer += {"sig", it->sig_};
      xferArray += thisXfer;
  }
  j += {XFER_TAG, xferArray};
  return(j.dump());
}

std::vector<uint8_t> DCTransaction::ToCBOR() const
{
  return(json::to_cbor(ToJSON()));
}

} //end namespace Devcash
