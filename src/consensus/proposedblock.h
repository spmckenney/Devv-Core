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
  /*ProposedBlock(const std::string& blockStr,
      int blockHeight, const KeyRing& keys);
  ProposedBlock(const std::vector<Transaction>& txs,
      const Validation& vs,
      unsigned int blockHeight);
  ProposedBlock(const std::vector<Transaction>& txs,
      const Validation& vs,
      FinalBlock previousBlock);
  virtual ~ProposedBlock() {};*/

  void setBlockHeight(unsigned int blockHeight) {
    block_height_=blockHeight;
  }

  bool addTransaction(std::string txStr, KeyRing& keys);

  bool validateBlock(const KeyRing& keys) const;
  bool signBlock(EC_KEY* eckey, std::string myAddr);

};

typedef std::shared_ptr<ProposedBlock> ProposedPtr;

} /* namespace Devcash */

#endif /* CONSENSUS_PROPOSEDBLOCK_H_ */
