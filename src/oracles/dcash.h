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
#include "consensus/blockchain.h"

using namespace Devv;

class dcash : public oracleInterface {

 public:

  dcash(std::string data) : oracleInterface(data) {};

/**
 *  @return the string name that invokes this oracle
 */
  static std::string getOracleName() {
    return("io.devv.dcash");
  }

/**
 *  @return the shard used by this oracle
 */
  static uint64_t getShardIndex() {
    return(0);
  }

/**
 *  @return the coin type used by this oracle
 */
  static uint64_t getCoinIndex() {
    return(0);
  }

/** Checks if this proposal is objectively sound according to this oracle.
 *  When this function returns false, the proposal is syntactically unsound
 *  and will be invalid for all chain states.
 * @return true iff the proposal can be valid according to this oracle
 * @return false otherwise
 */
  bool isSound() override {
    InputBuffer buffer(Str2Bin(raw_data_));
    Tier2Transaction tx = Tier2Transaction::QuickCreate(buffer);
    for (const TransferPtr& xfer : tx.getTransfers()) {
      if (xfer->getDelay() != 0) {
        error_msg_ = "Dcash transactions may not have a delay.";
        return false;
      }
	}
    return true;
  }

/** Checks if this proposal is valid according to this oracle
 *  given a specific blockchain.
 * @params context the blockchain to check against
 * @return true iff the proposal is valid according to this oracle
 * @return false otherwise
 */
  bool isValid(const Blockchain& context) override {
    if (!isSound()) return false;
    InputBuffer buffer(Str2Bin(raw_data_));
    Tier2Transaction tx = Tier2Transaction::QuickCreate(buffer);
    ChainState last_state = context.getHighestChainState();
    for (const TransferPtr& xfer : tx.getTransfers()) {
      if (xfer->getAmount() < 0) {
        Address addr = xfer->getAddress();
        if (last_state.getAmount(dnerowallet::getCoinIndex(), addr) > 0) {
          error_msg_ =  "Error: Dnerowallets may not send dcash.";
          return false;
        } //endif has dnerowallet
        break;
      }
	}
    return true;
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
    if (!isValid(context)) return out;
    InputBuffer buffer(Str2Bin(raw_data_));
    Tier2Transaction tx = Tier2Transaction::QuickCreate(buffer);
    std::vector<Tier2Transaction> txs;
    txs.push_back(std::move(tx));
    std::pair<uint64_t, std::vector<Tier2Transaction>> p(getShardIndex(), std::move(txs));
    out.insert(std::move(p));
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
    InputBuffer buffer(Str2Bin(raw_data_));
    Tier2Transaction tx = Tier2Transaction::QuickCreate(buffer);
    return tx.getJSON();
  }

/** Generate the appropriate signature(s) for this proposal.
 *
 * @params address - the address corresponding to this key
 * @params key - an ECDSA key, AES encrypted with ASCII armor
 * @params aes_password - the AES password for the key
 * @return the signed oracle data
 */
  std::string Sign(std::string address
        , std::string key, std::string aes_password) override {
    InputBuffer buffer(Str2Bin(raw_data_));
    Tier2Transaction unsigned_tx = Tier2Transaction::QuickCreate(buffer);
    EC_KEY* key_ptr = LoadEcKey(address, key, aes_password);
    std::vector<Transfer> xfers;
    for (const auto& xfer : unsigned_tx.getTransfers()) {
      InputBuffer xfer_buffer(xfer->getCanonical());
      Transfer copy_xfer(xfer_buffer);
      xfers.push_back(copy_xfer);
    }
    Tier2Transaction the_tx(unsigned_tx.getOperation()
             , xfers
             , unsigned_tx.getNonce()
             , key_ptr);
    return Bin2Str(the_tx.getCanonical());
  }

private:
 std::string error_msg_;

};

#endif /* ORACLES_DCASH_H_ */
