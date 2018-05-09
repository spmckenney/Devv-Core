/*
 * KeyRing.h definted key management for Devcash.
 *
 *  Created on: Mar 3, 2018
 *      Author: Nick Williams
 */

#ifndef CONSENSUS_KEYRING_H_
#define CONSENSUS_KEYRING_H_

#include "common/devcash_context.h"
#include "primitives/Transfer.h"

namespace Devcash {

class KeyRing {
 public:
  KeyRing(DevcashContext& context);
  virtual ~KeyRing() {};

  Address InsertAddress(std::string hex, EC_KEY* key);
  EC_KEY* getKey(const Address& addr) const;
  bool isINN(const Address& addr) const;
  Address getInnAddr() const;
  int CountNodes() const;
  int CountWallets() const;
  int getNodeIndex(const Address& addr) const;
  Address getNodeAddr(int index) const;
  Address getWalletAddr(int index) const;
  EC_KEY* getNodeKey(int index) const;
  EC_KEY* getWalletKey(int index) const;
  std::vector<Address> getDesignatedWallets(int index) const;

 private:
  KeyRing();
  KeyRing& operator=(KeyRing&);
  KeyRing(KeyRing&);

  DevcashContext context_;
  std::map<Address, EC_KEY*> key_map_;
  std::vector<Address> node_list_;
  std::vector<Address> wallet_list_;
  Address inn_addr_;
};

} /* namespace Devcash */

#endif /* CONSENSUS_KEYRING_H_ */
