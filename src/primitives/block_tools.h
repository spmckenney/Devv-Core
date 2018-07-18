/*
 * primitives/block_tools.h
 *
 *  Created on: July 2, 2018
 *      Author: Shawn McKenney
 */
#pragma once

#include "primitives/FinalBlock.h"

namespace Devcash {

/** Checks if binary is encoding a block
 * @note this function is pretty heuristic, do not use in critical cases
 * @return true if this data encodes a block
 * @return false otherwise
 */
bool IsBlockData(const std::vector<byte>& raw);

/** Checks if binary is encoding Transactions
 * @note this function is pretty heuristic, do not use in critical cases
 * @return true if this data encodes Transactions
 * @return false otherwise
 */
bool IsTxData(const std::vector<byte>& raw);

/** Checks if two chain state maps contain the same state
 * @return true if the maps have the same state
 * @return false otherwise
 */
bool CompareChainStateMaps(const std::map<Address, std::map<uint64_t, int64_t>>& first,
                           const std::map<Address, std::map<uint64_t, int64_t>>& second);

/** Dumps the map inside a chainstate object into a human-readable JSON format.
 * @return a string containing a description of the chain state.
 */
std::string WriteChainStateMap(const std::map<Address, std::map<uint64_t, int64_t>>& map);

/**
 * Create a Tier1Transaction from a FinalBlock
 * Throws on error
 *
 * @param block Valid FinalBlock
 * @param keys KeyRing for new transaction
 * @return unique_ptr to new Tier1Transaction
 */
Tier1TransactionPtr CreateTier1Transaction(const FinalBlock& block, const KeyRing& keys);

} // namespace Devcash
