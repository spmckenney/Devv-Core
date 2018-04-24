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

KeyRing::KeyRing(DevcashContext& context)
  : context_(context), key_map_(), inn_addr_()
{
  CASH_TRY {
     EVP_MD_CTX* ctx;
         if(!(ctx = EVP_MD_CTX_create())) {
           LOG_FATAL << "Could not create signature context!";
           CASH_THROW("Could not create signature context!");
         }

     std::vector<byte> msg = {'h', 'e', 'l', 'l', 'o'};
     Hash test_hash;
     test_hash = dcHash(msg);

     EC_KEY* inn_key = loadEcKey(ctx,
         context_.kINN_ADDR,
         context_.kINN_KEY);

     Signature sig;
     SignBinary(inn_key, test_hash, sig);

     if (!VerifyByteSig(inn_key, test_hash, sig)) {
       LOG_FATAL << "Invalid INN key!";
       return;
     }

     std::vector<byte> inn_addr(Hex2Bin(context_.kINN_ADDR));
     std::copy_n(inn_addr_.begin(), kADDR_SIZE, inn_addr.begin());
     std::pair<std::vector<byte>, EC_KEY*> new_pair(inn_addr, inn_key);
     key_map_.insert(new_pair);

     for (unsigned int i=0; i<context_.kADDRs.size(); i++) {

       EC_KEY* addr_key = loadEcKey(ctx,
           context_.kADDRs[i],
           context_.kADDR_KEYs[i]);

       SignBinary(addr_key, test_hash, sig);
       if (!VerifyByteSig(addr_key, test_hash, sig)) {
         LOG_WARNING << "Invalid address["+std::to_string(i)+"] key!";
         CASH_THROW("Invalid address["+std::to_string(i)+"] key!");
       }

       std::vector<byte> addr(Hex2Bin(context_.kADDRs[i]));
       std::pair<std::vector<byte>, EC_KEY*> addr_pair(addr, addr_key);
       key_map_.insert(addr_pair);
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

       std::vector<byte> node_addr(Hex2Bin(context_.kNODE_ADDRs[i]));
       std::pair<std::vector<byte>, EC_KEY*> node_pair(node_addr, addr_key);
       key_map_.insert(node_pair);
     }
     LOG_INFO << "Crypto Keys initialized.";
   } CASH_CATCH (const std::exception& e) {
     LOG_WARNING << FormatException(&e, "transaction");
   }

}

EC_KEY* KeyRing::getKey(const Address& addr) const {
  std::vector<byte> addr_vec(std::begin(addr), std::end(addr));
  auto it = key_map_.find(addr_vec);
  if (it != key_map_.end()) return it->second;
  LOG_WARNING << "Key for is missing!\n";
  CASH_THROW("Key for is missing!");
}

bool KeyRing::isINN(const Address& addr) const {
  return(inn_addr_ == addr);
}

EC_KEY* KeyRing::getNodeKey(int index) const {
  auto it = key_map_.find(Hex2Bin(context_.kNODE_ADDRs[index]));
  if (it != key_map_.end()) return it->second;
  LOG_WARNING << "Node["+std::to_string(index)+"] key is missing!\n";
  CASH_THROW("Node["+std::to_string(index)+"] key is missing!");
}


} /* namespace Devcash */
