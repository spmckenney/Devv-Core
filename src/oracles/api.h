/*
 * api.h is an oracle to handle permissions for external extensions to
 * these oracles.
 *
 *  Created on: Mar 1, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_API_H_
#define ORACLES_API_H_

#include <string>

#include "oracleInterface.h"
#include "common/logger.h"
#include "consensus/chainstate.h"
#include "primitives/Transaction.h"

using namespace Devcash;

class DCapi : public oracleInterface {

 public:

  const long kAPI_LIFETIME = 31449600; //1 year in seconds

  /**
   *  @return string that invokes this oracle in a T2 transaction
  */
  static std::string getCoinType() {
    return("api");
  }

  /**
   *  @return int internal index of this coin type
  */
  static int getCoinIndex() {
    return(5);
  }

  /** Checks if a transaction is objectively valid according to this oracle.
   *  When this function returns false, a transaction is syntactically invalid
   *  and will be invalid for all chain states.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  API tokens may not be exchanged.
   *
   * @params checkTx the transaction to (in)validate
   * @return true iff the transaction can be valid according to this oracle
   * @return false otherwise
   */
  bool isSound(Transaction checkTx) {
    if (checkTx.getOperation() == eOpType::Exchange) return false;
    return true;
  }

  /** Checks if a transaction is valid according to this oracle
   *  given a specific chain state.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  An address can only have 1 API token.
   *
   * @params checkTx the transaction to (in)validate
   * @params context the chain state to check against
   * @return true iff the transaction is valid according to this oracle
   * @return false otherwise
   */
  bool isValid(Transaction checkTx, ChainState& context) {
    if (!isSound(checkTx)) return false;
    std::vector<Transfer> xfers = checkTx.getTransfers();
    for (auto it=xfers.begin(); it != xfers.end(); ++it) {
      int64_t amount = it->getAmount();
      if (amount > 0) {
        Address addr = it->getAddress();
        if ((context.getAmount(getCoinIndex(), addr) > 0)) {
          LOG_WARNING << "Error: Addr already has an API token.";
          return false;
        } //endif has API
      } //endif receiver
    } //end transfer loop
    return true;
  }

/** Generate a tier 1 smartcoin transaction based on this tier 2 transaction.
 *
 * @pre This transaction must be valid.
 * @params checkTx the transaction to (in)validate
 * @return a tier 1 transaction to implement this tier 2 logic.
 */
  Transaction getT1Syntax(Transaction theTx) {
    Transaction out(theTx);
    //if (out.delay_ == 0) out.delay_ = kAPI_LIFETIME;
    return(out);
  }

/**
 * End-to-end tier2 process that takes a string, parses it into a transaction,
 * validates it against the provided chain state,
 * and returns a tier 1 transaction if it is valid.
 * Returns null if the transaction is invalid.
 *
 * An API being created or modified must have an INN reference, "apiRef".
 *
 * @params rawTx the raw transaction to process
 * @params context the chain state to check against
 * @return a tier 1 transaction to implement this tier 2 logic.
 * @return empty/null transaction if the transaction is invalid
 */
  Transaction Tier2Process(std::vector<byte> rawTx,
      ChainState context, const KeyRing& keys) {
    Transaction tx(rawTx, keys);
    if (!isValid(tx, context)) {
      return tx;
    }
    if (tx.getOperation() == 0 || tx.getOperation() == 1) {
      //TODO: check api reference string in nonce and verify reference with INN
    }
    return tx;
  }

};

#endif /* ORACLES_API_H_ */
