/*
 * KeyRing.cpp implements key management for Devcash.
 *
 *  Created on: Mar 3, 2018
 *      Author: Nick Williams
 */

#include "KeyRing.h"

#include "common/ossladapter.h"
#include "common/util.h"

#include <map>

namespace Devcash {

std::map<std::vector<byte>, EC_KEY*> keyMap_;
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

    std::vector<byte> msg = {'h', 'e', 'l', 'l', 'o'};
    Hash test_hash(dcHash(msg));
    std::string sDer;

    EC_KEY* inn_key = loadEcKey(ctx,
        context_.kINN_ADDR,
        context_.kINN_KEY);

    Signature sig;
    SignBinary(inn_key, test_hash, sig);

    if (!VerifyByteSig(inn_key, test_hash, sig)) {
      LOG_FATAL << "Invalid INN key!";
      CASH_THROW("Invalid INN key!");
    }

    keyMap_.at(Str2Bin(context_.kINN_ADDR)) = inn_key;

    for (unsigned int i=0; i<context_.kADDRs.size(); i++) {

      EC_KEY* addr_key = loadEcKey(ctx,
          context_.kADDRs[i],
          context_.kADDR_KEYs[i]);

      SignBinary(addr_key, test_hash, sig);
      if (!VerifyByteSig(addr_key, test_hash, sig)) {
        LOG_WARNING << "Invalid address["+std::to_string(i)+"] key!";
        CASH_THROW("Invalid address["+std::to_string(i)+"] key!");
      }

      keyMap_.at(Str2Bin(context_.kADDRs[i])) = addr_key;
    }

    for (unsigned int i=0; i<context_.kNODE_ADDRs.size(); i++) {

      EC_KEY* addr_key = loadEcKey(ctx,
          context_.kNODE_ADDRs[i],
          context_.kNODE_KEYs[i]);

      SignBinary(addr_key, test_hash, sig);
      if (!VerifyByteSig(addr_key, test_hash, sig)) {
        LOG_WARNING << "Invalid node["+std::to_string(i)+"] key!";
        CASH_THROW("Invalid node["+std::to_string(i)+"] key!");
      }

      keyMap_.at(Str2Bin(context_.kNODE_ADDRs[i])) = addr_key;
    }
    is_init_ = true;
    return is_init_;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "transaction");
    return false;
  }
}

EC_KEY* KeyRing::getKey(const Address& addr) const {
  if (addr[0] == 114) {
    auto it = keyMap_.find(Str2Bin(context_.kINN_ADDR));
    if (it != keyMap_.end()) return it->second;
    LOG_WARNING << "INN key is missing!\n";
    CASH_THROW("INN key is missing!");
  }

  std::vector<byte> addr_vec(std::begin(addr), std::end(addr));

  auto it = keyMap_.find(addr_vec);
  if (it != keyMap_.end()) return it->second;
  LOG_WARNING << "Key for is missing!\n";
  CASH_THROW("Key for is missing!");
}

bool KeyRing::isINN(const Address& addr) const {
  return(addr[0] == 114);
}

EC_KEY* KeyRing::getNodeKey(int index) const {
  auto it = keyMap_.find(Str2Bin(context_.kNODE_ADDRs[index]));
  if (it != keyMap_.end()) return it->second;
  LOG_WARNING << "Node["+std::to_string(index)+"] key is missing!\n";
  CASH_THROW("Node["+std::to_string(index)+"] key is missing!");
}


} /* namespace Devcash */
