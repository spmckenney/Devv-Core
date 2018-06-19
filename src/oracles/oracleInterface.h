/*
 * validation.h defines an interface for primitize Devcash oracles.
 * Devcash oracles further specify validation logic for particular
 * transaction types beyond the 'abstract' smartcoin transactions.
 * Note that smartcoin is not abstract as a C++ class,
 * rather it is a logical abstraction for all tokenized transactions.
 *
 *  Created on: Feb 23, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLE_INTERFACE_H_
#define ORACLE_INTERFACE_H_

#include <string>
#include "consensus/chainstate.h"
#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"

namespace Devcash
{

class oracleInterface {

 public:

  /** Constructor/Destructor */
  oracleInterface() {};
  virtual ~oracleInterface() {};

/**
 *  @return string that invokes this oracle in a T2 transaction
 */
  static std::string getCoinType() {
    return("SmartCoin");
  };

  static int getCoinIndexByType(std::string coinType) {
    if (coinType == "dcash" || coinType == "dnero" || coinType == "0") {
      return(0);
    } else if (coinType == "dneroavailable"  || coinType == "1") {
      return(1);
    } else if (coinType == "dnerowallet" || coinType == "2") {
      return(2);
    } else if (coinType == "id" || coinType == "3") {
      return(3);
    } else if (coinType == "vote" || coinType == "4") {
      return(4);
    } else if (coinType == "api" || coinType == "5") {
      return(5);
    } else if (coinType == "data" || coinType == "6") {
      return(6);
    } else { //invalid/unknown type
      return(-1);
    }
  }

  static std::string getCoinTypeByIndex(int coinIndex) {
    switch (coinIndex) {
      case 0: return "dnero";
      case 1: return "dneroavailable";
      case 2: return "dnerowallet";
      case 3: return "id";
      case 4: return "vote";
      case 5: return "api";
      case 6: return "data";
    }
    return "invalid";
  }

/**
 *  @note external coin types do not have an index and must be decomposed
 *  into a set of internal types by the oracle
 *  @return the index of this coin type
 */
  static uint64_t getCoinIndex();

/** Checks if a transaction is objectively sound according to this oracle.
 *  When this function returns false, a transaction is syntactically unsound
 *  and will be invalid for all chain states.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 * @params checkTx the transaction to (in)validate
 * @return true iff the transaction can be valid according to this oracle
 * @return false otherwise
 */
  virtual bool isSound(Transaction& checkTx) = 0;

/** Checks if a transaction is valid according to this oracle
 *  given a specific chain state.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 * @params checkTx the transaction to (in)validate
 * @params context the chain state to check against
 * @return true iff the transaction is valid according to this oracle
 * @return false otherwise
 */
  virtual bool isValid(Transaction& checkTx, const ChainState& context) = 0;

/** Generate a tier 1 smartcoin transaction based on this tier 2 transaction.
 *
 * @pre This transaction must be valid.
 * @params checkTx the transaction to (in)validate
 * @return a tier 1 transaction to implement this tier 2 logic.
 */
  virtual Tier1TransactionPtr getT1Syntax(Tier2TransactionPtr) = 0;

/**
 * End-to-end tier2 process that takes a string, parses it into a transaction,
 * validates it against the provided chain state,
 * and returns a tier 1 transaction if it is valid.
 * Returns null if the transaction is invalid.
 *
 * @params input_buffer the raw transaction to process
 * @params context the chain state to check against
 * @return a tier 1 transaction to implement this tier 2 logic
 * @return nullptr if the transaction is invalid
 */
  virtual Tier2TransactionPtr tier2Process(InputBuffer &input_buffer,
                                           const ChainState &context,
                                           const KeyRing &keys) = 0;

};

} //end namespace Devcash

#endif /* ORACLE_INTERFACE_H_ */
