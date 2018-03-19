/*
 * validation.cpp implements the structure of the validation section of a block.
 *
 *  Created on: Jan 3, 2018
 *  Author: Nick Williams
 */

#include "validation.h"

#include <stdint.h>

#include "../common/json.hpp"
#include "../common/logger.h"
#include "../common/ossladapter.h"
#include "../common/util.h"

namespace Devcash
{

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

DCValidationBlock::DCValidationBlock(std::string jsonMsg) {
  CASH_TRY {
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
  : sigs_(other.sigs_) {
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

DCValidationBlock::DCValidationBlock(vmap) {
  CASH_TRY {
    //sigs(sigMap);
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
