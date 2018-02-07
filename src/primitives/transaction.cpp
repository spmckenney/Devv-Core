/*
 * transaction.cpp
 *
 *  Created on: Dec 11, 2017
 *      Author: Nick Williams
 */

#include <stdint.h>
#include "transaction.h"
#include "../ossladapter.h"
#include "../tinyformat.h"
#include "../util.h"
#include "../utilstrencodings.h"
#include "../json.hpp"
using json = nlohmann::json;

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

struct OpString
{
    std::string jsonVal;
    OpType flag;
};

const OpString OpTypes[] =
{
    {"create", OpType::Create},
    {"modify", OpType::Modify},
	{"exchange", OpType::Exchange},
    {"delete", OpType::Delete}
};

DCTransfer::DCTransfer(std::string jsonTx) {
	CASH_TRY {
		json j = json::parse(jsonTx);
		addr = j["addr"].dump();
		amount = atoi(j["amount"].dump());
		nonce = 0;
		if (!j["nonce"].empty()) nonce = atoi(j["nonce"].dump());
		sig = "";
		if (!j["sig"].empty()) {
			sig = j["sig"].dump();
			sig.erase(remove(sig.begin(), sig.end(), '\"'), sig.end());
		}
	} CASH_CATCH (const std::exception* e) {
		FormatException(e, "xfer", DCLog::ALL);
	}
}

DCTransfer::DCTransfer() {
	addr = "";
	amount = 0;
	nonce = 0;
	sig = "";
}

DCTransfer::DCTransfer(const char* cbor) {
	CASH_TRY {
		addr = "";
		amount = 0;
		nonce = 0;
		sig = "";
	} CASH_CATCH (const std::exception& e) {
		LogPrintStr("Failed to parse xfer cbor.");
	}
}

DCTransfer::DCTransfer(const DCTransfer &other) {
	if (this != &other) {
		this->addr = other.addr;
		this->amount = other.amount;
		this->nonce = other.nonce;
		this->sig = other.sig;
	}
}

std::string DCTransfer::getCanonical()
{
	std::string out("\"addr\":\""+addr+"\",\"amount\":"+std::to_string(amount)+",\"nonce\":"+std::to_string(nonce));
	return (out);
}

OpType getOpType(std::string oper) {
	if ("create"==oper) return(OpType::Create);
	if ("modify"==oper) return(OpType::Modify);
	if ("exchange"==oper) return(OpType::Exchange);
	if ("delete"==oper) return(OpType::Delete);
	CASH_THROW("Invalid operation: "+oper);
}

DCTransaction::DCTransaction() {
	xfers = nullptr;
	oper = OpType::Create;
	type = "SmartCoin";
}

DCTransaction::DCTransaction(std::string jsonTx) {
	oper = OpType::Create;
	type = "SmartCoin";
	xfers = nullptr;
	CASH_TRY {
		json jsonObj = json::parse(jsonTx);
		std::string opTemp = jsonObj[OPER_TAG];
		oper = getOpType(opTemp);
		type = jsonObj[TYPE_TAG];
		xfers = new std::vector<DCTransfer>(0);
		for (auto iter = jsonObj[XFER_TAG].begin(); iter != jsonObj[XFER_TAG].end(); ++iter) {
			std::string xfer = iter.value().dump();
			DCTransfer* t = new DCTransfer(xfer);
			xfers->push_back(*t);
		}
		hash = ComputeHash();
		jsonStr = jsonObj.dump();
	} CASH_CATCH (const std::exception* e) {
		FormatException(e, "transaction", DCLog::ALL);
	}
}

DCTransaction::DCTransaction(char* cbor) {
	oper = OpType::Create;
	type = "SmartCoin";
	hash = ComputeHash();
	xfers = nullptr;
}

DCTransaction::DCTransaction(const DCTransaction& tx) : xfers(tx.xfers) {
	oper = tx.oper;
	type = tx.type;
	hash = tx.hash;
}

bool DCTransaction::isValid(EC_KEY* eckey) {
	CASH_TRY {
		long nValueOut = 0;
		for (std::vector<DCTransfer>::iterator it = xfers->begin(); it != xfers->end(); ++it) {
			nValueOut += it->amount;
			if ((oper == OpType::Delete && it->amount > 0) ||
					(oper != OpType::Delete && it->amount < 0) || oper == OpType::Modify) {
				if (it->nonce < 1) {
					LogPrintStr("Error: nonce is required for outgoing tx");
					return(false);
				}
				std::string msg("\"addr\":"+it->addr+",\"amount\":"+std::to_string(it->amount)+",\"nonce\":"+std::to_string(it->nonce));
				//TODO: hash transfer before signing?
				//char hash[SHA256_DIGEST_LENGTH*2+1];
				//dcHash(msg, hash);
				if (!verifySig(eckey, msg, it->sig)) {
					LogPrintStr("Error: signature did not validate");
					return(false);
				}
			}
		}
		if (nValueOut != 0) {
			LogPrintStr("Error: transaction amounts are asymmetric");
			return(false);
		}
		return(true);
	} CASH_CATCH (const std::exception* e) {
		FormatException(e, "transaction", DCLog::ALL);
	}
	return(false);
}

long DCTransaction::GetValueOut() const
{
    long nValueOut = 0;
    for (std::vector<DCTransfer>::iterator it = xfers->begin(); it != xfers->end(); ++it) {
    	nValueOut += it->amount;
    }
    return nValueOut;
}

unsigned int DCTransaction::GetByteSize() const
{
    return jsonStr.size();
}

std::string DCTransaction::ComputeHash() const
{
	return strHash(ToJSON());
}

std::string DCTransaction::ToJSON() const
{
    json j = {{OPER_TAG, oper},{TYPE_TAG, type}};
    json xferArray = json::array();
    for (std::vector<DCTransfer>::iterator it = xfers->begin(); it != xfers->end(); ++it) {
    	json thisXfer = {{"addr", it->addr}, {"amount", it->amount}};
    	if (it->nonce > 0) thisXfer += {"nonce", it->nonce};
    	if (it->sig.length() > 0) thisXfer += {"sig", it->sig};
    	xferArray += thisXfer;
    }
    j += {XFER_TAG, xferArray};
	return(j.dump());
}

std::vector<uint8_t> DCTransaction::ToCBOR() const
{
	return(json::to_cbor(ToJSON()));
}


