/*
 * data.h is an oracle to handle data storage on chain.
 *
 *  Created on: Mar 1, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_DATA_H_
#define ORACLES_DATA_H_

#include <string>

#include "oracleInterface.h"
#include "common/logger.h"
#include "consensus/chainstate.h"

using namespace Devcash;

class DCdata : public oracleInterface {

 public:

  const int kBYTES_PER_COIN = 10240; //1 year in seconds

  /**
   *  @return string that invokes this oracle in a T2 transaction
  */
  static std::string getCoinType() {
    return("data");
  }

  /**
   *  @return int internal index of this coin type
  */
  static int getCoinIndex() {
    return(6);
  }

  /** Checks if a transaction is objectively valid according to this oracle.
   *  When this function returns false, a transaction is syntactically invalid
   *  and will be invalid for all chain states.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  A data transaction can only store 10 kB per data token on chain.
   *
   * @params checkTx the transaction to (in)validate
   * @return true iff the transaction can be valid according to this oracle
   * @return false otherwise
   */
  bool isSound(Transaction& checkTx) override {
    if (checkTx.getOperation() == eOpType::Exchange) {
      //TODO: check that exchange is to an INN data collection address
      //TODO: check that nonce size is valid for coins expended
      /*if (checkTx.getValueOut() < int(checkTx.nonce_.size()/kBYTES_PER_COIN)) {
        LOG_WARNING << "Error: Data are too large for tokens provided.";
        return false;
      }*/
    }
    return true;
  }

  /** Checks if a transaction is valid according to this oracle
   *  given a specific chain state.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   * @params checkTx the transaction to (in)validate
   * @params context the chain state to check against
   * @return true iff the transaction is valid according to this oracle
   * @return false otherwise
   */
  bool isValid(Transaction& checkTx, const ChainState&) override {
    if (!isSound(checkTx)) return false;
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
    Tier2TransactionPtr tx = std::make_unique<Tier2Transaction>(input_buffer, keys);
    if (!isValid(*tx, context)) {
      return std::move(tx);
    }
    return nullptr;
  }

};

#endif /* ORACLES_DATA_H_ */
