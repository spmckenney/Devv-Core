/*
 * oracleInterface.h defines an interface for primitize Devv oracles.
 * Devv oracles further specify validation logic for particular
 * transaction types beyond the 'abstract' smartcoin transactions.
 * Oracles form a Directed Acyclic Graph (DAG) that decomposes abstract
 * functions into a set of clear Tier2Transactions. Oracles can be written
 * externally by 3rd parties and announce transactions to the Devv.io system
 * in the form of composed oracle transactions or simple Tier2Transactions.
 * Note that smartcoin is not abstract as a C++ class,
 * rather it is a logical abstraction for all tokenized transactions.
 *
 * @copywrite  2018 Devvio Inc
 *
 */

#ifndef ORACLE_INTERFACE_H_
#define ORACLE_INTERFACE_H_

#include <string>
#include "common/binary_converters.h"
#include "consensus/blockchain.h"
#include "primitives/Tier2Transaction.h"
#include "devv.pb.h"

namespace Devv
{

class oracleInterface {

 public:

  /** Constructor/Destructor */
  oracleInterface(std::vector<byte> data) : raw_data_(Bin2Str(data)) {};
  oracleInterface(std::string data) : raw_data_(data) {};
  virtual ~oracleInterface() {};

/**
 *  @return the string name that invokes this oracle
 */
  static std::string getOracleName();
  virtual std::string getInstanceName();

/**
 *  @return the shard used by this oracle
 */
  static uint64_t getShardIndex();

/**
 *  @return the coin type used by this oracle
 */
  static uint64_t getCoinIndex();

/** Checks if this proposal is objectively sound according to this oracle.
 *  When this function returns false, the proposal is syntactically unsound
 *  and will be invalid for all chain states.
 * @return true iff the proposal can be valid according to this oracle
 * @return false otherwise
 */
  virtual bool isSound() = 0;

/** Checks if this proposal is valid according to this oracle
 *  given a specific blockchain.
 * @params context the blockchain to check against
 * @return true iff the proposal is valid according to this oracle
 * @return false otherwise
 */
  virtual bool isValid(const Blockchain& context) = 0;

/**
 *  @return if not valid or not sound, return an error message
 */
  virtual std::string getErrorMessage() = 0;

/** This method returns all of the fully decomposed transactions generated by
 * this oracle previously given its proposal and current context, including
 * relevant blockchains and the decomposition depth of this instance. The
 * length of this map equals getCurrentDepth().
 * For a new proposal with no transactions on chain, this map is empty.
 *
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a map of previous transactions generated by this oracle.
 */
  virtual std::map<uint64_t, std::vector<Tier2Transaction>>
    getTrace(const Blockchain& context) = 0;

/** Determines the current depth of this oracle given its contextual blockchain.
 *
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a number indicating the current iteration/depth of this oracle's DAG
 */
  virtual uint64_t getCurrentDepth(const Blockchain& context) = 0;

/** This method returns all of the fully decomposed transactions generated by
 * this oracle given its proposal and current context, including the relevant
 * blockchain and the decomposition depth of this instance. In some cases this
 * map may be empty (for example, a contract results in a contingency to do
 * nothing). The first element of the pair indicates the shard where each
 * transaction should be encoded. All of these transactions will be encoded at
 * roughly the same time as they are announced to the shard peers simultaneously.
 *
 * @params context the blockchain of the shard that provides context for this oracle
 * @params keys provides INN keys to sign new transactions
 * @return a map of next transactions generated by this oracle.
 */
  virtual std::map<uint64_t, std::vector<Tier2Transaction>>
    getNextTransactions(const Blockchain& context, const KeyRing& keys) = 0;

/** Recursively generate the state of this oracle and all dependent oracles.
 *
 * @pre This proposal must be valid.
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a map of oracles to data
 */
  virtual std::map<std::string, std::vector<byte>>
    getDecompositionMap(const Blockchain& context) = 0;

/** Recursively generate the state of this oracle and all dependent oracles.
 *
 * @pre This proposal must be valid.
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a map of oracles to data encoded in JSON
 */
  virtual std::map<std::string, std::string>
    getDecompositionMapJSON(const Blockchain& context) = 0;

/** Generate the proposal message to sign for this oracle.
 *
 * @return the oracle porposal to sign
 */
  virtual std::vector<byte> Sign() = 0;

/**
 * @return the internal state of this oracle
 */
  std::vector<byte> getCanonical() { return Str2Bin(raw_data_); }

/**
 * @return the internal state of this oracle in JSON.
 */
  virtual std::string getJSON() = 0;

 protected:
  /**
   * Default Constructor
   */
  oracleInterface() = default;

  //using string to align with protobuf type
  std::string raw_data_;

};

} //end namespace Devv

#endif /* ORACLE_INTERFACE_H_ */
