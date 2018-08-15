/*
 * dnerowallet.h is an oracle to require dnero transactions.
 *
 *  Created on: Feb 23, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_DNEROWALLET_H_
#define ORACLES_DNEROWALLET_H_

#include <string>

#include "oracleInterface.h"
#include "common/logger.h"
#include "consensus/chainstate.h"
#include "primitives/Transaction.h"

using namespace Devcash;

class dnerowallet : public oracleInterface {

 public:

/**
 *  @return the string name that invokes this oracle
 */
  static std::string getOracleName() {
    return("io.devv.dnerowallet");
  }

/**
 *  @return the shard used by this oracle
 */
  static uint64_t getShardIndex() {
    return(2);
  }

/**
 *  @return the coin type used by this oracle
 */
  static uint64_t getCoinIndex() {
    return(2);
  }

/** Checks if this proposal is objectively sound according to this oracle.
 *  When this function returns false, the proposal is syntactically unsound
 *  and will be invalid for all chain states.
 * @return true iff the proposal can be valid according to this oracle
 * @return false otherwise
 */
  bool isSound() override {
    return false;
  }

/** Checks if this proposal is valid according to this oracle
 *  given a specific blockchain.
 * @params context the blockchain to check against
 * @return true iff the proposal is valid according to this oracle
 * @return false otherwise
 */
  bool isValid(const Blockchain& context) override {
    return false;
  }

/**
 *  @return if not valid or not sound, return an error message
 */
  std::string getErrorMessage() override {
    return(error_msg_);
  }

/** Generate the transactions to encode the effect of this propsal on chain.
 *
 * @pre This transaction must be valid.
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a map of shard indicies to transactions to encode in each shard
 */
  std::map<uint64_t, std::vector<Tier2Transaction>>
      getTransactions(const Blockchain& context) override {
    std::map<uint64_t, std::vector<Tier2Transaction>> out;
    return out;
  }

/** Recursively generate the state of this oracle and all dependent oracles.
 *
 * @pre This transaction must be valid.
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a map of oracles to data
 */
  std::map<std::string, std::vector<byte>>
      getDecompositionMap(const Blockchain& context) override {
    std::map<std::string, std::vector<byte>> out;
    std::vector<byte> data(Str2Bin(raw_data_));
    std::pair<std::string, std::vector<byte>> p(getOracleName(), data);
    out.insert(p);
    return out;
  }

/** Recursively generate the state of this oracle and all dependent oracles.
 *
 * @pre This transaction must be valid.
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a map of oracles to data encoded in JSON
 */
  virtual std::map<std::string, std::string>
      getDecompositionMapJSON(const Blockchain& context) override {
    std::map<std::string, std::string> out;
    std::pair<std::string, std::string> p(getOracleName(), getJSON());
    out.insert(p);
    return out;
  }

/**
 * @return the internal state of this oracle in JSON.
 */
  std::string getJSON() override {
    return "";
  }

private:
 std::string error_msg_;

};

#endif /* ORACLES_DNEROWALLET_H_ */
