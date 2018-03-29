/*
 * blockmanager.cpp implements high-level block operations
 *
 *  Created on: Jan 22, 2018
 *  Author: Nick Williams
 */



#include <stdint.h>
#include <vector>

#include "primitives/transaction.h"
#include "primitives/validation.h"
#include "primitives/block.h"

namespace Devcash
{

DCBlock createBlock(const std::vector<DCTransaction>& txs,
    const std::vector<DCValidationBlock>& vs) {

}

bool validateBlock(DCBlock& block) {

}

DCBlock finalizeBlock(const std::vector<DCTransaction>& txs,
    const std::vector<DCValidationBlock>& vs) {

}

std::string computeMerkleRoot(const std::vector<DCTransaction>& txs,
    const std::vector<DCValidationBlock>& vs) {

}

} //end namespace Devcash
