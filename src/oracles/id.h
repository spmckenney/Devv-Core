/*
 * id.h is an oracle to track identities.
 *
 *  Created on: Feb 28, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_ID_H_
#define ORACLES_ID_H_

#include <string>

#include "oracleInterface.h"
#include "common/logger.h"
#include "consensus/chainstate.h"
#include "primitives/Transaction.h"

using namespace Devcash;

class DCid : public oracleInterface {

 public:

  const long kID_LIFETIME = 31449600; //1 year in seconds

  /**
   *  @return string that invokes this oracle in a T2 transaction
  */
  static std::string getCoinType() {
    return("id");
  }

  /**
   *  @return int internal index of this coin type
  */
  static int getCoinIndex() {
    return(3);
  }

  /** Checks if a transaction is objectively valid according to this oracle.
   *  When this function returns false, a transaction is syntactically invalid
   *  and will be invalid for all chain states.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  IDs may not be exchanged.
   *
   * @params checkTx the transaction to (in)validate
   * @return true iff the transaction can be valid according to this oracle
   * @return false otherwise
   */
  bool isValid(Transaction checkTx) {
    if (checkTx.oper_ == eOpType::Exchange) return false;
    return true;
  }

  /** Checks if a transaction is valid according to this oracle
   *  given a specific chain state.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  An address can only have 1 ID token.
   *
   * @params checkTx the transaction to (in)validate
   * @params context the chain state to check against
   * @return true iff the transaction is valid according to this oracle
   * @return false otherwise
   */
  bool isValid(Transaction checkTx, DCState& context) {
    if (!isValid(checkTx)) return false;
    for (std::vector<Transfer>::iterator it=checkTx.xfers_.begin();
        it != checkTx.xfers_.end(); ++it) {
      if (it->amount_ > 0) {
        if ((context.getAmount(DCid::getCoinIndex(), it->addr_)) > 0) {
          LOG_WARNING << "Error: Addr already has an ID token.";
          return false;
        } //endif has ID
      } //endif receiver
    } //end transfer loop
    return true;
  }

/** Generate a tier 1 smartcoin transaction based on this tier 2 transaction.
 *
 * @note if delay is zero, sets to default delay as side-effect.
 *
 * @pre This transaction must be valid.
 * @params checkTx the transaction to (in)validate
 * @return a tier 1 transaction to implement this tier 2 logic.
 */
  Transaction getT1Syntax(Transaction theTx) {
    Transaction out(theTx);
    //if (out.delay_ == 0) out.delay_ = kID_LIFETIME;
    return(out);
  }

/**
 * End-to-end tier2 process that takes a string, parses it into a transaction,
 * validates it against the provided chain state,
 * and returns a tier 1 transaction if it is valid.
 * Returns null if the transaction is invalid.
 *
 * An ID being created or modified must have an INN reference, "idRef".
 *
 * @params rawTx the raw transaction to process
 * @params context the chain state to check against
 * @return a tier 1 transaction to implement this tier 2 logic.
 * @return empty/null transaction if the transaction is invalid
 */
  Transaction Tier2Process(std::vector<byte> rawTx,
      DCState context) {
    Transaction tx(rawTx);
    if (!isValid(tx, context)) {
      return tx;
    }
    if (tx.oper_ == 0 || tx.oper_ == 1) {
      /*if (jsonObj["idRef"].empty()) {
        LOG_WARNING << "Error: No INN reference for this ID.";
        return tx;
      }*/
      //TODO: verify reference in nonce with INN
      //if (tx.delay_ == 0) tx.delay_ = kID_LIFETIME;
    }
    return tx;
  }

};

#endif /* ORACLES_ID_H_ */
