/*
 * transaction.cpp
 *
 *  Created on: Dec 11, 2017
 *      Author: Nick Williams
 */

#ifndef DEVCASH_CONSENSUS_PARAMS_H
#define DEVCASH_CONSENSUS_PARAMS_H

#include <uint256.h>
#include <limits>
#include <map>
#include <string>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    uint256 defaultAssumeValid;
};
} // namespace Consensus

#endif // DEVCASH_CONSENSUS_PARAMS_H
