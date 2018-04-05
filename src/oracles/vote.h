/*
 * vote.h is an oracle to handle arbitrary election processes.
 *
 *  Created on: Feb 28, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_VOTE_H_
#define ORACLES_VOTE_H_

#include <string>

#include "oracleInterface.h"
#include "common/logger.h"
#include "consensus/chainstate.h"
#include "primitives/transaction.h"

//TODO: the election creates the vote tokens by giving them to the voters

namespace Devcash
{

class DCVote : public oracleInterface {

 public:

  std::string election_;
  std::vector<std::string>* targets_;

/** Constructors/Destructors */
  DCVote();
  DCVote(std::string election, std::vector<std::string>* targets);
  ~DCVote();

  /**
   *  @return string that invokes this oracle in a T2 transaction
  */
  static std::string getCoinType() {
    return("vote");
  }

  /**
   *  @return int internal index of this coin type
  */
  static int getCoinIndex() {
    return(4);
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
  bool isValid(Devcash::DCTransaction checkTx);

  /** Checks if a transaction is valid according to this oracle
   *  given a specific chain state.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  A voter must have enough votes for this election and only vote for
   *  target candidates specified when this election was created.
   *
   * @params checkTx the transaction to (in)validate
   * @params context the chain state to check against
   * @return true iff the transaction is valid according to this oracle
   * @return false otherwise
   */
  bool isValid(Devcash::DCTransaction checkTx, Devcash::DCState& context);

/** Generate a tier 1 smartcoin transaction based on this tier 2 transaction.
 *
 * @pre This transaction must be valid.
 * @params checkTx the transaction to (in)validate
 * @return a tier 1 transaction to implement this tier 2 logic.
 * @return if transaction is invalid, may return nullptr
 */
  Devcash::DCTransaction getT1Syntax(Devcash::DCTransaction theTx);

/**
 * End-to-end tier2 process that takes a string, parses it into a transaction,
 * validates it against the provided chain state,
 * and returns a tier 1 transaction if it is valid.
 * Returns null if the transaction is invalid.
 *
 * @pre if this is an exchange/vote operation, this DCVote object
 * must previously been initialized with the last create/modify operation.
 * @params rawTx the raw transaction to process
 * @params context the chain state to check against
 * @return a tier 1 transaction to implement this tier 2 logic.
 * @return empty/null transaction if the transaction is invalid
 */
  Devcash::DCTransaction Tier2Process(std::string rawTx,
      Devcash::DCState context);

};

} //end namespace Devcash

#endif /* ORACLES_VOTE_H_ */
