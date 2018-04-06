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

  FinalBlock(const std::vector<DCTransaction>& txs,
      const DCValidationBlock& vs,
      unsigned int blockHeight);
  FinalBlock(const std::string& blockStr,
      int blockHeight, const KeyRing& keys);
  FinalBlock();
  FinalBlock(const DCBlock& other, unsigned int blockHeight);
  FinalBlock(const FinalBlock& other);
  virtual ~FinalBlock() {};

  bool validateBlock(const KeyRing &keys);

};

} /* namespace Devcash */

#endif /* CONSENSUS_FINALBLOCK_H_ */
