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

ProposedBlock::ProposedBlock(const std::vector<DCTransaction>& txs,
    const DCValidationBlock& vs,
    unsigned int blockHeight)
    : DCBlock(txs, vs)
    , block_height_(blockHeight)
{
}

ProposedBlock::ProposedBlock(const std::string& blockStr,
    int blockHeight, const KeyRing& keys)
    :  DCBlock(blockStr, keys)
    , block_height_(blockHeight)
{
}

ProposedBlock::ProposedBlock(const std::vector<DCTransaction>& txs,
    const DCValidationBlock& vs,
    FinalBlock previousBlock)
    : DCBlock(txs, vs)
    , block_height_(previousBlock.block_height_+1) {
}

bool ProposedBlock::addTransaction(std::string txStr, KeyRing& keys) {
  LOG_DEBUG << "Add transaction: "+txStr;
  return DCBlock::addTransaction(txStr, keys);
}

bool ProposedBlock::validateBlock(const KeyRing& keys) const {
  return(DCBlock::validate(keys));
}

bool ProposedBlock::signBlock(EC_KEY* eckey, std::string myAddr) {
  return(DCBlock::signBlock(eckey, myAddr));
}

} /* namespace Devcash */
