/*
 * dnero.h is an oracle to validate reversible dnero transactions.
 *
 *  Created on: Feb 23, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_DNERO_H_
#define ORACLES_DNERO_H_

#include <string>

#include "dneroavailable.h"
#include "dnerowallet.h"
#include "oracleInterface.h"
#include "common/logger.h"
#include "consensus/chainstate.h"

using namespace Devcash;

class dnero : public oracleInterface {

 public:

  const long kDEFAULT_DELAY = 604800; //1 week in seconds

  /**
   *  @return string that invokes this oracle in a T2 transaction
  */
  static std::string getCoinType() {
    return("dnero");
  }

  /**
   *  @return int internal index of this coin type
  */
  static int getCoinIndex() {
    return(0);
  }

  /** Checks if a transaction is objectively valid according to this oracle.
   *  When this function returns false, a transaction is syntactically invalid
   *  and will be invalid for all chain states.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   * @params checkTx the transaction to (in)validate
   * @return true iff the transaction can be valid according to this oracle
   * @return false otherwise
   */
  bool isSound(Transaction&) override {
    return true;
  }

  /** Checks if a transaction is valid according to this oracle
   *  given a specific chain state.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  A dnero sender must have a dnerowallet or dneroavailable token.
   *
   * @params checkTx the transaction to (in)validate
   * @params context the chain state to check against
   * @return true iff the transaction is valid according to this oracle
   * @return false otherwise
   */
  bool isValid(Transaction& checkTx, const ChainState& context) override {
    if (!isSound(checkTx)) return false;
    std::vector<Transfer> xfers = checkTx.getTransfers();
    for (auto it=xfers.begin(); it != xfers.end(); ++it) {
      if (it->getAmount() < 0) {
        Address addr = it->getAddress();
        if ((context.getAmount(dnerowallet::getCoinIndex(), addr) < 1) &&
            (context.getAmount(dneroavailable::getCoinIndex(), addr) < 1)) {
          LOG_WARNING << "Error: Addr has no dnerowallet or dneroavailable.";
          return false;
        } //endif has dnerowallet or dneroavailable
      } //endif sender
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
  Tier1TransactionPtr getT1Syntax(Tier2TransactionPtr) override {
    // TODO(spm)
    Tier1TransactionPtr out = std::make_unique<Tier1Transaction>();
    //if (out.delay_ == 0) out.delay_ = kDEFAULT_DELAY;
    //out.type_ = dcash::getCoinType();
    return(out);
  }

/**
 * End-to-end tier2 process that takes a string, parses it into a transaction,
 * validates it against the provided chain state,
 * and returns a tier 1 transaction if it is valid.
 * Returns null if the transaction is invalid.
 *
 * @params input_buffer the raw transaction to process
 * @params context the chain state to check against
 * @return a tier 1 transaction to implement this tier 2 logic.
 * @return empty/null transaction if the transaction is invalid
 */
  Tier2TransactionPtr tier2Process(InputBuffer &input_buffer,
                                   const ChainState &context,
                                   const KeyRing &keys) override {
    Tier2TransactionPtr tx = Tier2Transaction::CreateUniquePtr(input_buffer, keys);
    if (!isValid(*tx, context)) {
      return std::move(tx);
    }
    return nullptr;
  }

};

#endif /* ORACLES_DNERO_H_ */
