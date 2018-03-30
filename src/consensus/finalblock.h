/*
 * finalblock.h
 *
 *  Created on: Mar 10, 2018
 *      Author: Nick Williams
 */

#ifndef CONSENSUS_FINALBLOCK_H_
#define CONSENSUS_FINALBLOCK_H_

#include "chainstate.h"
#include "primitives/block.h"

namespace Devcash {

class FinalBlock : public DCBlock {
 public:
  unsigned int block_height_ = 0;

  FinalBlock(std::vector<DCTransaction>& txs,
      DCValidationBlock& vs,
      unsigned int blockHeight);
  FinalBlock();
  FinalBlock(DCBlock& other, unsigned int blockHeight);
  FinalBlock(const FinalBlock& other);
  virtual ~FinalBlock() {};

  bool validateBlock(KeyRing &keys);

};

} /* namespace Devcash */

#endif /* CONSENSUS_FINALBLOCK_H_ */
