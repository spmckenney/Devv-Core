/*
 * KeyRing.cpp implements key management for Devcash.
 *
 *  Created on: Mar 3, 2018
 *      Author: Nick Williams
 */

#include "KeyRing.h"

#include <string>

#include "common/ossladapter.h"

#include <unordered_map>

namespace Devcash {

std::unordered_map<std::string, EC_KEY*> keyMap_;
DevcashContext context_;

KeyRing::KeyRing(DevcashContext& context)
  : context_(context), is_init_(false)
{
}

bool KeyRing::initKeys() {
  CASH_TRY {
    EVP_MD_CTX* ctx;
        if(!(ctx = EVP_MD_CTX_create())) {
          LOG_FATAL << "Could not create signature context!";
          CASH_THROW("Could not create signature context!");
        }

    std::string hash(strHash("hello"));
    std::string sDer;

    EC_KEY* inn_key = loadEcKey(ctx,
        context_.kINN_ADDR,
        context_.kINN_KEY);

    if (!verifySig(inn_key, hash, sign(inn_key, hash))) {
      LOG_FATAL << "Invalid INN key!";
      CASH_THROW("Invalid INN key!");
    }

    keyMap_.insert(std::pair<std::string, EC_KEY*>(context_.kINN_ADDR, inn_key));

    for (unsigned int i=0; i<context_.kADDRs.size(); i++) {

      EC_KEY* addr_key = loadEcKey(ctx,
          context_.kADDRs[i],
          context_.kADDR_KEYs[i]);

      if (!verifySig(addr_key, hash, sign(addr_key, hash))) {
        LOG_WARNING << "Invalid address["+std::to_string(i)+"] key!";
        CASH_THROW("Invalid address["+std::to_string(i)+"] key!");
      }

      keyMap_.insert(std::pair<std::string, EC_KEY*>(context_.kADDRs[i], addr_key));
    }

    for (unsigned int i=0; i<context_.kNODE_ADDRs.size(); i++) {

      EC_KEY* addr_key = loadEcKey(ctx,
          context_.kNODE_ADDRs[i],
          context_.kNODE_KEYs[i]);

      if (!verifySig(addr_key, hash, sign(addr_key, hash))) {
        LOG_WARNING << "Invalid node["+std::to_string(i)+"] key!";
        CASH_THROW("Invalid node["+std::to_string(i)+"] key!");
      }

      keyMap_.insert(std::pair<std::string, EC_KEY*>(context_.kNODE_ADDRs[i], addr_key));
    }
    is_init_ = true;
    return is_init_;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "transaction");
    return false;
  }
}

EC_KEY* KeyRing::getKey(const std::string& addr) const {
  if (addr[0] == '7') {
    auto it = keyMap_.find(context_.kINN_ADDR);
    if (it != keyMap_.end()) return it->second;
    LOG_WARNING << "INN key is missing!\n";
    CASH_THROW("INN key is missing!");
  }

  auto it = keyMap_.find(addr);
  if (it != keyMap_.end()) return it->second;
  LOG_WARNING << "Key for '"+addr+"'is missing!\n";
  CASH_THROW("Key for '"+addr+"'is missing!");
}

bool KeyRing::isINN(const std::string& addr) const {
  return(addr[0] == '7');
}

EC_KEY* KeyRing::getNodeKey(int index) const {
  auto it = keyMap_.find(context_.kNODE_ADDRs[index]);
  if (it != keyMap_.end()) return it->second;
  LOG_WARNING << "Node["+std::to_string(index)+"] key is missing!\n";
  CASH_THROW("Node["+std::to_string(index)+"] key is missing!");
}


} /* namespace Devcash */
