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
#include "../common/json.hpp"
#include "../common/logger.h"
#include "../consensus/chainstate.h"
#include "../primitives/transaction.h"

using json = nlohmann::json;

class DCapi : public oracleInterface {

 public:

  const long kAPI_LIFETIME = 31449600; //1 year in seconds

  /**
   *  @return string that invokes this oracle in a T2 transaction
  */
  static std::string getCoinType() {
    return("api");
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
  bool isValid(Devcash::DCTransaction checkTx) {
    if (checkTx.isOpType("exchange")) return false;
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
  bool isValid(Devcash::DCTransaction checkTx, Devcash::DCState& context) {
    if (!isValid(checkTx)) return false;
    for (std::vector<Devcash::DCTransfer>::iterator it=checkTx.xfers_->begin();
        it != checkTx.xfers_->end(); ++it) {
      if (it->amount_ > 0) {
        if ((context.getAmount(getCoinType(), it->addr_) > 0)) {
          LOG_WARNING << "Error: Addr already has an API token.";
          return false;
        } //endif has API
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
  Devcash::DCTransaction getT1Syntax(Devcash::DCTransaction theTx) {
    Devcash::DCTransaction out(theTx);
    if (out.delay_ == 0) out.delay_ = kAPI_LIFETIME;
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
  Devcash::DCTransaction Tier2Process(std::string rawTx,
      Devcash::DCState context) {
    json jsonObj = json::parse(rawTx);
    Devcash::DCTransaction tx(jsonObj);
    if (!isValid(tx, context)) {
      tx.setNull();
      return tx;
    }
    if (tx.isOpType("create") || tx.isOpType("modify")) {
      if (jsonObj["apiRef"].empty()) {
        LOG_WARNING << "Error: No INN reference for this API.";
        tx.setNull();
        return tx;
      }
      //TODO: verify reference with INN
      if (tx.delay_ == 0) tx.delay_ = kAPI_LIFETIME;
    }
    return tx;
  }

};

#endif /* ORACLES_API_H_ */
