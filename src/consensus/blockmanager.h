/*
 * blockmanager.h definies high-level block operations
 *
 *  Created on: Dec 22, 2017
 *  Author: Nick Williams
 */

#ifndef SRC_CONSENSUS_BLOCKMANAGER_H_
#define SRC_CONSENSUS_BLOCKMANAGER_H_

#include <stdint.h>
#include <vector>

#include "../primitives/transaction.h"
#include "../primitives/validation.h"
#include "../primitives/block.h"

namespace Devcash
{

DCBlock createBlock(const std::vector<DCTransaction>& txs,
    const std::vector<DCValidationBlock>& vs);
bool validateBlock(DCBlock& block);
DCBlock finalizeBlock(const std::vector<DCTransaction>& txs,
    const std::vector<DCValidationBlock>& vs);
std::string computeMerkleRoot(const std::vector<DCTransaction>& txs,
    const std::vector<DCValidationBlock>& vs);

} //end namespace Devcash

#endif /* SRC_CONSENSUS_BLOCKMANAGER_H_ */
