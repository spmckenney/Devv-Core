/*
 * dcash.h is an oracle to validate basic dcash transactions.
 *
 *  Created on: Feb 23, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_DCASH_H_
#define ORACLES_DCASH_H_

#include <string>

#include "dnerowallet.h"
#include "oracleInterface.h"
#include "common/logger.h"
#include "consensus/chainstate.h"

using namespace Devcash;

class dcash : public oracleInterface {

 public:

  /**
   *  @return string that invokes this oracle in a T2 transaction
  */
  static std::string getCoinType() {
    return("dcash");
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
   *
   *  dcash transactions must be immediate with a delay of 0.
   *
   * @params checkTx the transaction to (in)validate
   * @return true iff the transaction can be valid according to this oracle
   * @return false otherwise
   */
  bool isSound(Tier2Transaction& checkTx) override {
    std::vector<TransferPtr> xfers = checkTx.getTransfers();
    for (auto& it : xfers) {
      if (it->getDelay() != 0) {
        LOG_WARNING << "Error: Delays are not allowed for dcash.";
        return false;
      }
    } //end for each transfer
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
  bool isValid(Tier2Transaction& checkTx, const ChainState& context) override {
    if (!isSound(checkTx)) { return false; }
    std::vector<TransferPtr> xfers = checkTx.getTransfers();
    for (auto& it : xfers) {
      if (it->getAmount() < 0) {
        Address addr = it->getAddress();
        if (context.getAmount(dnerowallet::getCoinIndex(), addr) > 0) {
          LOG_WARNING << "Error: Dnerowallets may not send dcash.";
          return false;
        } //endif has dnerowallet
      } //endif sender
    } //end transfer loop
    return true;
  }

/** Generate a tier 1 smartcoin transaction based on this tier 2 transaction.
 *
 * @pre This transaction must be valid.
 * @params checkTx the transaction to (in)validate
 * @return a tier 1 transaction to implement this tier 2 logic.
 */
  Tier1TransactionPtr getT1Syntax(Tier2TransactionPtr) override {
    // TODO(spm)
    Tier1TransactionPtr t1 = std::make_unique<Tier1Transaction>();
    return(t1);
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

#endif /* ORACLES_DCASH_H_ */
