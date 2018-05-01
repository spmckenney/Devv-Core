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
  EC_KEY* getNodeKey(int index) const;

 private:
  DevcashContext context_;
  std::map<Address, EC_KEY*> key_map_;
  std::vector<Address> node_list_;
  Address inn_addr_;
};

} /* namespace Devcash */

#endif /* CONSENSUS_KEYRING_H_ */
