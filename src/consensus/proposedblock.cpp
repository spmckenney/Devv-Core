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

ProposedBlock::ProposedBlock(std::vector<DCTransaction>& txs,
    DCValidationBlock& vs,
    unsigned int blockHeight)
    : DCBlock(txs, vs)
    , block_height_(blockHeight)
{
}

ProposedBlock::ProposedBlock(std::string blockStr,
    int blockHeight, KeyRing& keys)
    :  block_height_(blockHeight)
    , DCBlock(blockStr, keys)
{
}

ProposedBlock::ProposedBlock(std::vector<DCTransaction>& txs,
    DCValidationBlock& vs,
    FinalBlock previousBlock)
    : DCBlock(txs, vs)
    , block_height_(previousBlock.block_height_+1) {
}

bool ProposedBlock::addTransaction(DCTransaction newTx, KeyRing& keys) {
  if (newTx.isValid(DCBlock::block_state_, keys, DCBlock::vals_.summaryObj_)) {
    DCBlock::vtx_.push_back(newTx);
  } else {
    LOG_WARNING << "Invalid transaction:"+newTx.ToJSON();
    return false;
  }
  return true;
}

bool ProposedBlock::validateBlock(KeyRing& keys) {
  return(!DCBlock::validate(keys));
}

bool ProposedBlock::signBlock(EC_KEY* eckey, std::string myAddr) {
  return(DCBlock::signBlock(eckey, myAddr));
}

} /* namespace Devcash */
