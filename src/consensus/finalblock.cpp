/*
 * finalblock.cpp
 *
 *  Created on: Mar 10, 2018
 *      Author: Nick Williams
 */

#include "finalblock.h"

#include "proposedblock.h"

namespace Devcash {

FinalBlock::FinalBlock() :
    DCBlock(),
    block_height_(0)
{
}

FinalBlock::FinalBlock(const FinalBlock& other)
  : DCBlock()
  , block_height_(other.block_height_)
{
}

FinalBlock::FinalBlock(std::vector<DCTransaction>& txs,
    DCValidationBlock& vs,
    unsigned int blockHeight)
    : DCBlock(txs, vs)
    , block_height_(blockHeight) {
}

FinalBlock::FinalBlock(DCBlock& other, unsigned int blockHeight)
  : DCBlock(other.vtx_, other.vals_)
  , block_height_(blockHeight)
{
  DCBlock::copyHeaders(other);
}

bool FinalBlock::validateBlock(KeyRing& keys) {
  return (DCBlock::validate(keys));
}

} /* namespace Devcash */
