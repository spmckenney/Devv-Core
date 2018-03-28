/*
 * KeyRing.h definted key management for Devcash.
 *
 *  Created on: Mar 3, 2018
 *      Author: Nick Williams
 */

#ifndef CONSENSUS_KEYRING_H_
#define CONSENSUS_KEYRING_H_

#include "../common/devcash_context.h"
#include "../common/ossladapter.h"

namespace Devcash {

using namespace Devcash;

class KeyRing {
 public:
  KeyRing(DevcashContext& context);
  virtual ~KeyRing() {};

  bool initKeys();
  EC_KEY* getKey(std::string addr);
  bool isINN(std::string addr);
  EC_KEY* getNodeKey(int index);

 private:
  DevcashContext context_;
  bool is_init_ = false;
};

} /* namespace Devcash */

#endif /* CONSENSUS_KEYRING_H_ */
