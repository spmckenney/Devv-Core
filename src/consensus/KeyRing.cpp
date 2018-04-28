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

Address KeyRing::InsertAddress(std::string hex, EC_KEY* key) {
  Address to_insert;
  std::vector<byte> addr(Hex2Bin(hex));
  std::copy_n(addr.begin(), kADDR_SIZE, to_insert.begin());
  std::pair<Address, EC_KEY*> new_pair(to_insert, key);
  key_map_.insert(new_pair);
  return to_insert;
}

KeyRing::KeyRing(DevcashContext& context)
  : context_(context), key_map_(), node_list_(), inn_addr_()
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

     inn_addr_ = InsertAddress(context_.kINN_ADDR, inn_key);

     for (unsigned int i=0; i<context_.kADDRs.size(); i++) {

       EC_KEY* addr_key = loadEcKey(ctx,
           context_.kADDRs[i],
           context_.kADDR_KEYs[i]);

       SignBinary(addr_key, test_hash, sig);
       if (!VerifyByteSig(addr_key, test_hash, sig)) {
         LOG_WARNING << "Invalid address["+std::to_string(i)+"] key!";
         CASH_THROW("Invalid address["+std::to_string(i)+"] key!");
       }

       InsertAddress(context_.kADDRs[i], addr_key);
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

       Address node_addr = InsertAddress(context_.kNODE_ADDRs[i], addr_key);
       node_list_.push_back(node_addr);
     }
     LOG_INFO << "Crypto Keys initialized.";
   } CASH_CATCH (const std::exception& e) {
     LOG_WARNING << FormatException(&e, "transaction");
   }

}

EC_KEY* KeyRing::getKey(const Address& addr) const {
  auto it = key_map_.find(addr);
  if (it != key_map_.end()) return it->second;
  std::string hex(toHex(std::vector<byte>(std::begin(addr), std::end(addr))));
  LOG_WARNING << "Key for '"+hex+"' is missing!\n";
  CASH_THROW("Key for '"+hex+"' is missing!");
}

bool KeyRing::isINN(const Address& addr) const {
  return(inn_addr_ == addr);
}

EC_KEY* KeyRing::getNodeKey(int index) const {
  Address node_addr = node_list_.at(index);
  auto it = key_map_.find(node_addr);
  if (it != key_map_.end()) return it->second;
  LOG_WARNING << "Node["+std::to_string(index)+"] key is missing!\n";
  CASH_THROW("Node["+std::to_string(index)+"] key is missing!");
}


} /* namespace Devcash */
