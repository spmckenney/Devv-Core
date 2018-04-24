/*
 * KeyRing.h definted key management for Devcash.
 *
 *  Created on: Mar 3, 2018
 *      Author: Nick Williams
 */

#ifndef CONSENSUS_KEYRING_H_
#define CONSENSUS_KEYRING_H_

#include "common/devcash_context.h"
#include "common/ossladapter.h"
#include "primitives/Transfer.h"

namespace Devcash {

class KeyRing {
 public:
  KeyRing(DevcashContext& context);
  virtual ~KeyRing() {};

  bool initKeys();
  EC_KEY* getKey(const Address& addr) const;
  bool isINN(const Address& addr) const;
  EC_KEY* getNodeKey(int index) const;

 private:
  DevcashContext context_;
  std::map<std::vector<byte>, EC_KEY*> key_map_;
  Address inn_addr_;
};

} /* namespace Devcash */

#endif /* CONSENSUS_KEYRING_H_ */
