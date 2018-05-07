/*
 * dnerowallet.h is an oracle to require dnero transactions.
 *
 *  Created on: Feb 23, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_DNEROWALLET_H_
#define ORACLES_DNEROWALLET_H_

#include <string>

#include "oracleInterface.h"
#include "common/logger.h"
#include "consensus/chainstate.h"
#include "primitives/Transaction.h"

using namespace Devcash;

class dnerowallet : public oracleInterface {

 public:

  /**
   *  @return string that invokes this oracle in a T2 transaction
  */
  static std::string getCoinType() {
    return("dnerowallet");
  }

  /**
   *  @return int internal index of this coin type
  */
  static int getCoinIndex() {
    return(2);
  }

  /** Checks if a transaction is objectively valid according to this oracle.
   *  When this function returns false, a transaction is syntactically invalid
   *  and will be invalid for all chain states.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  dnerowallet is a flag with only 0 or 1 such tokens at any address.
   *
   * @params checkTx the transaction to (in)validate
   * @return true iff the transaction can be valid according to this oracle
   * @return false otherwise
   */
  bool isSound(Transaction checkTx) {
    std::vector<Transfer> xfers = checkTx.getTransfers();
    for (auto it=xfers.begin();
        it != xfers.end(); ++it) {
      if (it->getAmount() > 1) {
        LOG_WARNING << "Error: Can only have at most 1 dnerowallet token.";
        return false;
      }
    }
    return true;
  }

  /** Checks if a transaction is valid according to this oracle
   *  given a specific chain state.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  A dnerowallet token indicates that this address may not send dcash.
   *
   * @params checkTx the transaction to (in)validate
   * @params context the chain state to check against
   * @return true iff the transaction is valid according to this oracle
   * @return false otherwise
   */
  bool isValid(Transaction checkTx, ChainState&) {
    if (!isSound(checkTx)) return false;
    return true;
  }

/** Generate a tier 1 smartcoin transaction based on this tier 2 transaction.
 *
 * @pre This transaction must be valid.
 * @params checkTx the transaction to (in)validate
 * @return a tier 1 transaction to implement this tier 2 logic.
 */
  Transaction getT1Syntax(Transaction theTx) {
    return(theTx);
  }

/**
 * End-to-end tier2 process that takes a string, parses it into a transaction,
 * validates it against the provided chain state,
 * and returns a tier 1 transaction if it is valid.
 * Returns null if the transaction is invalid.
 *
 * @params rawTx the raw transaction to process
 * @params context the chain state to check against
 * @return a tier 1 transaction to implement this tier 2 logic.
 * @return empty/null transaction if the transaction is invalid
 */
  Tier2Transaction Tier2Process(std::vector<byte> rawTx,
      ChainState context, const KeyRing& keys) {
    Tier2Transaction tx(rawTx, keys);
    if (!isValid(tx, context)) {
      return tx;
    }
    return tx;
  }

};

#endif /* ORACLES_DNEROWALLET_H_ */
