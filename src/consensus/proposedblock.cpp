/*
 * proposedblock.cpp
 *
 *  Created on: Mar 10, 2018
 *      Author: Nick Williams
 */

#include "proposedblock.h"

#include "primitives/block.h"

namespace Devcash {

using namespace Devcash;

ProposedBlock::ProposedBlock() :
        DCBlock(),
        block_height_(0)
{
}

ProposedBlock::ProposedBlock(const ProposedBlock& other)
  : DCBlock(), block_height_(other.block_height_)
  , chain_state_(other.chain_state_)
{
}

ProposedBlock::ProposedBlock(std::string blockStr,
    int blockHeight, DCState chainState)
    :  DCBlock(blockStr)
    , block_height_(blockHeight)
    , chain_state_(chainState)
{
}

ProposedBlock::ProposedBlock(std::vector<DCTransaction>& txs,
    DCValidationBlock& vs,
    FinalBlock previousBlock,
    DCState chainState)
    : DCBlock(txs, vs)
    , block_height_(previousBlock.block_height_+1)
    , chain_state_(chainState) {

}

bool ProposedBlock::addTransaction(DCTransaction newTx, KeyRing& keys) {
  DCSummary summary;
  if (newTx.isValid(chain_state_, keys, summary)) {
    DCBlock::vtx_.push_back(newTx);
  }
  return true;
}

bool ProposedBlock::validateBlock(KeyRing& keys) {
  if (!DCBlock::validate(chain_state_, keys)) return false;
  return true;
}

} /* namespace Devcash */
