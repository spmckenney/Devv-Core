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
 *  Updated: Aug 13, 2018
 *  Author: Nick Williams
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

/** Generate the transactions to encode the effect of this propsal on chain.
 *
 * @pre This transaction must be valid.
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a map of shard indicies to transactions to encode in each shard
 */
  virtual std::map<uint64_t, std::vector<Tier2Transaction>>
    getTransactions(const Blockchain& context) = 0;

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

/** Generate the appropriate signature(s) for this proposal.
 *
 * @params address - the address corresponding to this key
 * @params key - an ECDSA key, AES encrypted with ASCII armor
 * @params aes_password - the AES password for the key
 * @return the signed oracle data
 */
  virtual std::string Sign(std::string address
          , std::string key, std::string aes_password) = 0;

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
