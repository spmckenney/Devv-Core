/*
 * proposedblock.h
 *
 *  Created on: Mar 10, 2018
 *      Author: Nick Williams
 */

#ifndef CONSENSUS_PROPOSEDBLOCK_H_
#define CONSENSUS_PROPOSEDBLOCK_H_

#include "primitives/block.h"
#include "primitives/smartcoin.h"
#include "finalblock.h"

namespace Devcash {

class ProposedBlock : public DCBlock {
public:
  unsigned int block_height_;

  ProposedBlock();
  ProposedBlock(std::string blockStr,
      int blockHeight, KeyRing& keys);
  ProposedBlock(std::vector<DCTransaction>& txs,
      DCValidationBlock& vs,
      unsigned int blockHeight);
  ProposedBlock(std::vector<DCTransaction>& txs,
      DCValidationBlock& vs,
      FinalBlock previousBlock);
  virtual ~ProposedBlock() {};

  void setBlockHeight(unsigned int blockHeight) {
    block_height_=blockHeight;
  }

  bool addTransaction(std::string txStr, KeyRing& keys);

  bool validateBlock(KeyRing& keys);
  bool signBlock(EC_KEY* eckey, std::string myAddr);

};

} /* namespace Devcash */

#endif /* CONSENSUS_PROPOSEDBLOCK_H_ */
