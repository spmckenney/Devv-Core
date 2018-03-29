/*
 * finalblock.cpp
 *
 *  Created on: Mar 10, 2018
 *      Author: Nick Williams
 */

#include "finalblock.h"

namespace Devcash {

FinalBlock::FinalBlock() :
    DCBlock(),
    block_height_(0)
{
}

FinalBlock::FinalBlock(const FinalBlock& other)
  : DCBlock()
  , block_height_(other.block_height_)
  , chain_state_(other.chain_state_)
{
}

FinalBlock::FinalBlock(std::vector<DCTransaction>& txs,
    DCValidationBlock& vs,
    unsigned int blockHeight)
    : DCBlock(txs, vs)
    , block_height_(blockHeight) {
}

FinalBlock::FinalBlock(FinalBlock& other)
  : DCBlock(other.vtx_, other.vals_)
  , block_height_(other.block_height_)
{
}

bool FinalBlock::validateBlock(KeyRing& keys) {
  if (!DCBlock::validate(keys)) return false;
  return true;
}

} /* namespace Devcash */
