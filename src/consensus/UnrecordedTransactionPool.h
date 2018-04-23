/*
 * UnrecordedTransactionPool.h
 *
 *  Created on: Apr 20, 2018
 *      Author: Nick Williams
 */

#ifndef CONSENSUS_UNRECORDEDTRANSACTIONPOOL_H_
#define CONSENSUS_UNRECORDEDTRANSACTIONPOOL_H_

#include <map>
#include <vector>

#include "primitives/FinalBlock.h"

namespace Devcash
{
typedef std::pair<uint8_t, Transaction> SharedTransaction;
typedef std::map<Signature, SharedTransaction> TxMap;

class UnrecordedTransactionPool {
 public:

/** Constrcutors */
  UnrecordedTransactionPool(const ChainState& prior)
     : txs_(), pending_proposal_(prior) {}

  UnrecordedTransactionPool(const std::vector<byte>& serial
      , const ChainState& prior, const KeyRing& keys)
      : txs_(), pending_proposal_(prior) {
    AddTransactions(serial, keys);
  }

  UnrecordedTransactionPool(const UnrecordedTransactionPool& other)
    : txs_(other.txs_), pending_proposal_(other.pending_proposal_) {}
  UnrecordedTransactionPool(const TxMap& map, const ChainState& prior)
    : txs_(map), pending_proposal_(prior) {}

  /** Adds Transactions to this pool.
   *  @note if the Transaction is invalid it will not be added,
   *    but other valid transactions will be added.
   *  @note if the Transaction is valid, but is already in the pool,
   *        its reference count will be incremented.
   *  @param txs a vector of Transactions to add.
   *  @params keys a KeyRing that provides keys for signature verification
   *  @return true iff all Transactions are valid and in the pool
  */
    bool AddTransactions(std::vector<byte> serial, const KeyRing& keys) {
      std::vector<Transaction> temp;
      size_t counter = 0;
      while (counter < serial.size()) {
        Transaction one_tx(serial, counter, keys);
        counter += one_tx.getByteSize();
      }
      return AddTransactions(temp, keys);
    }

/** Adds Transactions to this pool.
 *  @note if the Transaction is invalid it will not be added,
 *    but other valid transactions will be added.
 *  @note if the Transaction is valid, but is already in the pool,
 *        its reference count will be incremented.
 *  @param txs a vector of Transactions to add.
 *  @params keys a KeyRing that provides keys for signature verification
 *  @return true iff all Transactions are valid and in the pool
*/
  bool AddTransactions(std::vector<Transaction> txs, const KeyRing& keys) {
    bool all_good = true;
    int counter = 0;
    for (auto const& item : txs) {
      Signature sig = item.getSignature();
      auto it = txs_.find(sig);
      if (it != txs_.end()) {
        it->second.first++;
      } else if (item.isSound(keys)) {
        SharedTransaction pair((uint8_t) 1, item);
        txs_.insert(std::pair<Signature, SharedTransaction>(sig, pair));
        counter++;
      } else { //tx is unsound
        all_good = false;
      }
    }
    LOG_INFO << "Added "+std::to_string(counter)+" sound transactions.";
    LOG_INFO << std::to_string(txs_.size())+" transactions pending.";
    return all_good;
  }

  /** Adds and verifies Transactions for this pool.
   *  @note if *any* Transaction is invalid or unsound,
   *    this function breaks and returns false,
   *    so other Transactions may not be added or verified
   *  @param txs a vector of Transactions to add and verify
   *  @params state the chain state to validate against
   *  @params keys a KeyRing that provides keys for signature verification
   *  @params summary the Summary to update
   *  @return true iff all Transactions are valid and in the pool
   *  @return false if any of the Transactions are invalid or unsound
  */
  bool AddAndVerifyTransactions(std::vector<Transaction> txs, ChainState& state
      , const KeyRing& keys, Summary& summary) {
    for (auto const& item : txs) {
      Signature sig = item.getSignature();
      auto it = txs_.find(sig);
      bool valid = it->second.second.isValid(state, keys, summary);
      if (!valid) return false; //tx is invalid
      if (it != txs_.end()) {
        if (valid) {
          it->second.first++;
        }
      } else if (item.isSound(keys)) {
        SharedTransaction pair((uint8_t) 0, item);
        if (valid) pair.first++;
        txs_.insert(std::pair<Signature, SharedTransaction>(sig, pair));
      } else { //tx is unsound
        return false;
      }
    }
    return true;
  }

/** Returns a JSON string representing these Transactions
 *  @note pointer counts are not preserved.
 *  @return a JSON string representing these Transactions.
*/
  std::string getJSON() const {
    std::string out("[");
    bool isFirst = true;
    for (auto const& item : txs_) {
      if (isFirst) {
        isFirst = false;
      } else {
        out += ",";
      }
      out += item.second.second.getJSON();
    }
    out += "]";
    return out;
  }

/** Returns a bytestring of these Transactions
 *  @return a bytestring of these Transactions
*/
  std::vector<byte> getCanonical() const {
    std::vector<byte> serial;
    for (auto const& item : txs_) {
      std::vector<byte> temp(item.second.second.getCanonical());
      serial.insert(serial.end(), temp.begin(), temp.end());
    }
    return serial;
  }

  bool HasPendingTransactions() {
    return(!txs_.empty());
  }

  std::vector<byte> ProposeBlock(const Hash& prev_hash, const ChainState& prior_state
      , const KeyRing& keys, const DevcashContext& context) {
    ChainState new_state(prior_state);
    Summary summary;
    Validation validation;

    std::vector<Transaction> validated = CollectValidTransactions(new_state
        ,keys, summary);

    ProposedBlock new_proposal(prev_hash, validated, summary, validation
        , new_state);
    new_proposal.SignBlock(keys, context);
    pending_proposal_ = new_proposal;
    return pending_proposal_.getCanonical();
  }

  bool HasProposal() {
    return(!pending_proposal_.isNull());
  }

  bool ReverifyProposal(const ChainState& prior, const KeyRing& keys) {
    if (pending_proposal_.isNull()) return false;
    std::vector<Transaction> pending = pending_proposal_.getTransactions();
    Summary summary;
    ChainState new_state(prior);
    if (ReverifyTransactions(pending, new_state, keys, summary)) {
      return true;
    }
    pending_proposal_.setNull();
    LOG_WARNING << "ProposedBlock invalidated by FinalBlock!";
    return false;
  }

  bool CheckValidation(std::vector<byte> remote
      , const DevcashContext& context) {
    if (pending_proposal_.isNull()) return false;
    return pending_proposal_.CheckValidationData(remote, context);
  }

  const FinalBlock FinalizeLocalBlock(const KeyRing& keys) {
    const FinalBlock final_block(FinalizeBlock(pending_proposal_, keys));
    pending_proposal_.setNull();
    return final_block;
  }

  const FinalBlock FinalizeRemoteBlock(const std::vector<byte>& serial
      , const ChainState prior, const KeyRing& keys) {
    ProposedBlock temp(serial, prior, keys);
    return FinalizeBlock(temp, keys);
  }

  /** Remove unreferenced Transactions from the pool.
   *  @return the number of Transactions removed.
  */
  int GarbageCollect() {
    //TODO: delete old unrecorded Transactions periodically
    return 0;
  }

 private:
  TxMap txs_;
  ProposedBlock pending_proposal_;

  /** Verifies Transactions for this pool.
   *  @note this implementation is greedy in selecting Transactions
   *  @params state the chain state to validate against
   *  @params keys a KeyRing that provides keys for signature verification
   *  @params summary the Summary to update
   *  @return a vector of unrecorded valid transactions
  */
  std::vector<Transaction> CollectValidTransactions(ChainState& state
      , const KeyRing& keys, Summary& summary) {
    std::vector<Transaction> valid;
    for (auto& item : txs_) {
      if (item.second.second.isValid(state, keys, summary)) {
        valid.push_back(item.second.second);
        item.second.first++;
      }
    }
    return valid;
  }

  /** Reverifies Transactions for this pool.
   *  @note this function does *not* increment Transaction pointers
   *  @param txs a vector of Transactions to verify
   *  @params state the chain state to validate against
   *  @params keys a KeyRing that provides keys for signature verification
   *  @params summary the Summary to update
   *  @return true iff, all transactions are valid wrt provided chainstate
   *  @return false otherwise
  */
  bool ReverifyTransactions(std::vector<Transaction> txs, ChainState& state
      , const KeyRing& keys, Summary& summary) {
    for (auto const& item : txs) {
      if (!item.isValid(state, keys, summary)) {
        return false;
      }
    }
    return true;
  }

  bool RemoveTransactions(const ProposedBlock& proposed) {
    for (auto const& item : proposed.getTransactions()) {
      txs_.erase(item.getSignature());
    }
  }

  const FinalBlock FinalizeBlock(const ProposedBlock& proposal
      , const KeyRing& keys) {
    RemoveTransactions(proposal);
    FinalBlock final(proposal);
    return final;
  }

};

} //end namespace Devcash


#endif /* CONSENSUS_UNRECORDEDTRANSACTIONPOOL_H_ */
