/*
 * dnerowallet.h is an oracle to require dnero transactions.
 *
 *  Created on: Feb 23, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_DNEROAVAILABLE_H_
#define ORACLES_DNEROAVAILABLE_H_

#include <string>

#include "oracleInterface.h"
#include "../common/logger.h"
#include "../consensus/chainstate.h"
#include "../primitives/transaction.h"

class dneroavailable : public oracleInterface {

 public:

/**
 *  @return string that invokes this oracle in a T2 transaction
 */
  static std::string getCoinType() {
    return("dneroavailable");
  }

/** Checks if a transaction is objectively valid according to this oracle.
 *  When this function returns false, a transaction is syntactically invalid
 *  and will be invalid for all chain states.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 *
 *  dneroavailable is a flag with only 0 or 1 such tokens at any address.
 *
 * @params checkTx the transaction to (in)validate
 * @return true iff the transaction can be valid according to this oracle
 * @return false otherwise
 */
  bool isValid(Devcash::DCTransaction checkTx) {
    for (std::vector<Devcash::DCTransfer>::iterator it=checkTx.xfers_->begin();
        it != checkTx.xfers_->end(); ++it) {
      if (it->amount_ > 1) {
        LOG_WARNING << "Error: Can only have at most 1 dneroavailable token.";
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
 *  A dneroavilable token indicates that this address may send dnero or dcash.
 *
 * @params checkTx the transaction to (in)validate
 * @params context the chain state to check against
 * @return true iff the transaction is valid according to this oracle
 * @return false otherwise
 */
  bool isValid(Devcash::DCTransaction checkTx, Devcash::DCState& context) {
    if (!isValid(checkTx)) return false;
    return true;
  }

/** Generate a tier 1 smartcoin transaction based on this tier 2 transaction.
 *
 * @pre This transaction must be valid.
 * @params checkTx the transaction to (in)validate
 * @return a tier 1 transaction to implement this tier 2 logic.
 */
  Devcash::DCTransaction getT1Syntax(Devcash::DCTransaction theTx) {
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
  Devcash::DCTransaction Tier2Process(std::string rawTx,
      Devcash::DCState context) {
    Devcash::DCTransaction tx(rawTx);
    if (!isValid(tx, context)) {
      tx.setNull();
      return tx;
    }
    return tx;
  }

};


#endif /* ORACLES_DNEROAVAILABLE_H_ */
