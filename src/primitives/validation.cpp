/*
 * validation.cpp
 *
 *  Created on: Jan 3, 2018
 *      Author: Nick Williams
 */

#include <stdint.h>
#include "validation.h"
#include "../ossladapter.h"
#include "../tinyformat.h"
#include "../util.h"
#include "../utilstrencodings.h"
#include "../json.hpp"
using json = nlohmann::json;

static const std::string VAL_TAG = "val";

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

DCValidationBlock::DCValidationBlock(std::string jsonMsg) {
	CASH_TRY {
		json j = json::parse(jsonMsg);
		json temp = json::array();
		for (auto iter = j.begin(); iter != j.end(); ++iter) {
			std::pair<std::string, std::string> oneSig(iter.key(), iter.value());
			sigs.insert(oneSig);
			temp += json::object({iter.key(), iter.value()});
		}
		hash = ComputeHash();
		jsonStr = temp.dump();
	} CASH_CATCH (const std::exception& e) {
		LogPrintStr("Failed to parse validation json: "+jsonMsg);
	}
}

DCValidationBlock::DCValidationBlock() {
}

DCValidationBlock::DCValidationBlock(const char* cbor) {
	CASH_TRY {
	} CASH_CATCH (const std::exception& e) {
		LogPrintStr("Failed to parse validation cbor.");
	}
}

DCValidationBlock::DCValidationBlock(const DCValidationBlock& other) : sigs(other.sigs) {

}

DCValidationBlock::DCValidationBlock(std::string node, std::string sig) {
	CASH_TRY {
		std::pair<std::string, std::string> oneSig(node, sig);
		sigs.insert(oneSig);
		hash = ComputeHash();
		json j = {node, sig};
		jsonStr = j.dump();
	} CASH_CATCH (const std::exception& e) {
		LogPrintStr("Failed to parse validation: "+node+", "+sig);
	}
}

DCValidationBlock::DCValidationBlock(vmap sigMap) {
	CASH_TRY {
		//sigs(sigMap);
		hash = ComputeHash();
	} CASH_CATCH (const std::exception& e) {
		LogPrintStr("Failed to parse validation map with size "+sigs.size());
	}
}

bool DCValidationBlock::addValidation(std::string node, std::string sig) {
	CASH_TRY {
		std::pair<std::string, std::string> oneSig(node, sig);
		sigs.insert(oneSig);
		hash = ComputeHash();
		return(true);
	} CASH_CATCH (const std::exception& e) {
		LogPrintStr("Failed to parse validation: "+node+", "+sig);
	}
	return(false);
}

unsigned int DCValidationBlock::GetByteSize() const
{
    return ToCBOR().size();
}

unsigned int DCValidationBlock::GetValidationCount() const
{
    return sigs.size();
}

std::string DCValidationBlock::ComputeHash() const
{
	return strHash(ToJSON());
}

std::string DCValidationBlock::ToJSON() const
{
	json j = {};
	for (auto it : sigs) {
		json thisSig = {it.first, it.second};
		j += thisSig;
	}
	return(j.dump());
}

std::vector<uint8_t> DCValidationBlock::ToCBOR() const
{
	return(json::to_cbor(ToJSON()));
}


