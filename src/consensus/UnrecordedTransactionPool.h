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

#include "concurrency/TransactionCreationManager.h"
#include "primitives/FinalBlock.h"

namespace Devcash
{
typedef std::pair<uint8_t, TransactionPtr> SharedTransaction;
typedef std::map<Signature, SharedTransaction> TxMap;

class UnrecordedTransactionPool {
 public:

  /** Constrcutors */
  UnrecordedTransactionPool(const ChainState& prior, eAppMode mode
     , size_t max_tx_per_block)
     : txs_()
    , pending_proposal_(prior)
    , max_tx_per_block_(max_tx_per_block)
    , tcm_(mode)
    , mode_(mode)
  {
    LOG_DEBUG << "UnrecordedTransactionPool(const ChainState& prior)";
  }

  UnrecordedTransactionPool(const UnrecordedTransactionPool& other) = delete;

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
      LOG_DEBUG << "AddTransactions(std::vector<byte> serial, const KeyRing& keys)";
      MTR_SCOPE_FUNC();
      CASH_TRY {
        std::vector<TransactionPtr> temp;
        size_t counter = 0;
        while (counter < serial.size()) {
          //note that Transaction constructor advances counter by reference
          if (mode_ == eAppMode::T2) {
            TransactionPtr one_tx = std::make_unique<Tier2Transaction>(serial, counter, keys);
            if (one_tx->getByteSize() < Transaction::MinSize()) {
              LOG_WARNING << "Invalid transaction, dropping the remainder of input.";
              break;
            }
            temp.push_back(std::move(one_tx));
		  } else if (mode_ == eAppMode::T1) {
            TransactionPtr one_tx = std::make_unique<Tier1Transaction>(serial, counter, keys);
            if (one_tx->getByteSize() < Transaction::MinSize()) {
              LOG_WARNING << "Invalid transaction, dropping the remainder of input.";
              break;
            }
            temp.push_back(std::move(one_tx));
          }
        }
        return AddTransactions(std::move(temp), keys);
      } CASH_CATCH (const std::exception& e) {
        LOG_FATAL << FormatException(&e, "UnrecordedTransactionPool.AddTransactions()");
        return false;
      }
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
  bool AddTransactions(std::vector<TransactionPtr> txs, const KeyRing& keys) {
    LOG_DEBUG << "AddTransactions(std::vector<Transaction> txs, const KeyRing& keys)";
    MTR_SCOPE_FUNC();
    std::lock_guard<std::mutex> guard(txs_mutex_);
    bool all_good = true;
    CASH_TRY {
      int counter = 0;
      for (TransactionPtr& item : txs) {
        Signature sig = item->getSignature();
        auto it = txs_.find(sig);
        if (it != txs_.end()) {
          it->second.first++;
          LOG_DEBUG << "Transaction already in UTX pool, increment reference count.";
        } else if (item->isSound(keys)) {
          SharedTransaction pair((uint8_t) 1, std::move(item));
          txs_.insert(std::pair<Signature, SharedTransaction>(sig, std::move(pair)));
          if (num_cum_txs_ == 0) {
            LOG_NOTICE << "AddTransactions(): First transaction added to TxMap";
            timer_.reset();
            trace_ = std::make_unique<MTRScopedTrace>("timer", "lifetime1");
          }
          num_cum_txs_++;
          counter++;
        } else { //tx is unsound
          LOG_DEBUG << "Transaction is unsound.";
          all_good = false;
        }
      }
      LOG_INFO << "Added "+std::to_string(counter)+" sound transactions.";
      LOG_INFO << std::to_string(txs_.size())+" transactions pending.";
    } CASH_CATCH (const std::exception& e) {
      LOG_FATAL << FormatException(&e, "UnrecordedTransactionPool.AddTransactions()");
      all_good = false;
    }
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
  bool AddAndVerifyTransactions(std::vector<TransactionPtr> txs, ChainState& state
      , const KeyRing& keys, Summary& summary) {
    LOG_DEBUG << "AddAndVerifyTransactions()";
    MTR_SCOPE_FUNC();
    std::lock_guard<std::mutex> guard(txs_mutex_);
    for (TransactionPtr& item : txs) {
      Signature sig = item->getSignature();
      auto it = txs_.find(sig);
      bool valid = it->second.second->isValid(state, keys, summary);
      if (!valid) return false; //tx is invalid
      if (it != txs_.end()) {
        if (valid) {
          it->second.first++;
        }
      } else if (item->isSound(keys)) {
        SharedTransaction pair((uint8_t) 0, std::move(item));
        if (valid) pair.first++;
        txs_.insert(std::pair<Signature, SharedTransaction>(sig, std::move(pair)));
        if (num_cum_txs_ == 0) {
          LOG_NOTICE << "AddTransactions(): First transaction added to TxMap";
          timer_.reset();
          trace_ = std::make_unique<MTRScopedTrace>("timer", "lifetime2");
        }
        num_cum_txs_++;

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
    LOG_DEBUG << "getJSON()";
    MTR_SCOPE_FUNC();
    std::lock_guard<std::mutex> guard(txs_mutex_);
    std::string out("[");
    bool isFirst = true;
    for (auto const& item : txs_) {
      if (isFirst) {
        isFirst = false;
      } else {
        out += ",";
      }
      out += item.second.second->getJSON();
    }
    out += "]";
    return out;
  }

/** Returns a bytestring of these Transactions
 *  @return a bytestring of these Transactions
*/
  std::vector<byte> getCanonical() const {
    LOG_DEBUG << "getCanonical()";
    MTR_SCOPE_FUNC();
    std::vector<byte> serial;
    std::lock_guard<std::mutex> guard(txs_mutex_);
    for (auto const& item : txs_) {
      std::vector<byte> temp(item.second.second->getCanonical());
      serial.insert(serial.end(), temp.begin(), temp.end());
    }
    return serial;
  }

/** Checks if this pool has pending Transactions
 *  @return true, iff this pool has pending Transactions
 */
  bool HasPendingTransactions() const {
    LOG_DEBUG << "Number pending transactions: "+std::to_string(txs_.size());
    std::lock_guard<std::mutex> guard(txs_mutex_);
    auto empty = txs_.empty();
    return(!empty);
  }

  /**
   *  @return the number of pending Transactions in this pool
   */
  size_t NumPendingTransactions() const {
    std::lock_guard<std::mutex> guard(txs_mutex_);
    auto size = txs_.size();
    return(size);
  }

  /**
   *  Create a new ProposedBlock based on pending Transaction in this pool
   *  @param prev_hash - the hash of the previous block
   *  @param prior_state - the chainstate prior to this proposal
   *  @param keys - the directory of Addresses and EC keys
   *  @param context - the Devcash context of this shard
   *  @return true, iff this pool created a new ProposedBlock
   *  @return false, if anything went wrong
   */
  bool ProposeBlock(const Hash& prev_hash, const ChainState& prior_state
      , const KeyRing& keys, const DevcashContext& context) {
    LOG_DEBUG << "ProposeBlock()";
    MTR_SCOPE_FUNC();
    ChainState new_state(prior_state);
    Summary summary;
    Validation validation;

    auto validated = CollectValidTransactions(new_state
        ,keys, summary);

    ProposedBlock new_proposal(prev_hash, validated, summary, validation
        , new_state);
    new_proposal.signBlock(keys, context);
    std::lock_guard<std::mutex> proposal_guard(pending_proposal_mutex_);
    LOG_WARNING << "ProposeBlock(): canon size: " << new_proposal.getCanonical().size();
    pending_proposal_.shallowCopy(new_proposal);
    return true;
  }

  /**
   *  @return true, iff this pool has a ProposedBlock
   */
  bool HasProposal() {
    LOG_DEBUG << "HasProposal()";
    std::lock_guard<std::mutex> proposal_guard(pending_proposal_mutex_);
    return(!pending_proposal_.isNull());
  }

  /**
   *  @return a binary representation of this pool's ProposedBlock
   */
  std::vector<byte> getProposal() {
    LOG_DEBUG << "getProposal()";
    std::lock_guard<std::mutex> proposal_guard(pending_proposal_mutex_);
    return pending_proposal_.getCanonical();
  }

  /**
   *  Update this pool's ProposedBlock based on a new FinalBlock.
   *  @note some Transactions in the proposal may be removed if they are
   *        no longer valid
   *  @param prev_hash - the hash of the highest FinalBlock
   *  @param prior_state - the chainstate of the highest FinalBlock
   *  @param keys - the directory of Addresses and EC keys
   *  @return true, iff this pool updated its ProposedBlock
   *  @return false, if anything went wrong
   */
  bool ReverifyProposal(const Hash& prev_hash, const ChainState&, const KeyRing&) {
    LOG_DEBUG << "ReverifyProposal()";
    MTR_SCOPE_FUNC();
    std::lock_guard<std::mutex> proposal_guard(pending_proposal_mutex_);
    if (pending_proposal_.isNull()) return false;
    pending_proposal_.setPrevHash(prev_hash);
    return true;
    //The remainder of this function is unneeded when transactions are unique
    //use logic like below if Proposals will be created in advance
    //and Transactions may have already been finalized
    /*std::vector<Transaction> pending = pending_proposal_.getTransactions();
    Summary summary;
    ChainState new_state(prior);
    if (ReverifyTransactions(pending, new_state, keys, summary)) {
      return true;
    }
    pending_proposal_.setNull();
    LOG_WARNING << "ProposedBlock invalidated by FinalBlock!";
    return false;*/
  }

  /**
   *  Checks a remote validation against this pool's ProposedBlock
   *  @param remote - a binary representation of the remote Validation
   *  @param context - the Devcash context of this shard
   *  @return true, iff the Validation validates this pool's ProposedBlock
   *  @return false, otherwise including if this pool has no ProposedBlock,
   *                 the Validation is for a different ProposedBlock,
   *                 or the Validation signature did not verify
   */
  bool CheckValidation(std::vector<byte> remote
      , const DevcashContext& context) {
    LOG_DEBUG << "CheckValidation()";
    std::lock_guard<std::mutex> proposal_guard(pending_proposal_mutex_);
    if (pending_proposal_.isNull()) return false;
    return pending_proposal_.checkValidationData(remote, context);
  }

  /**
   *  Create a new FinalBlock based on this pool's ProposedBlock
   *  @pre ensure this HasProposal() == true before calling this function
   *  @note nullifies this pool's ProposedBlock as a side-effect
   *  @return a FinalBlock based on this pool's ProposedBlock
   */
  const FinalBlock FinalizeLocalBlock() {
    LOG_DEBUG << "FinalizeLocalBlock()";
    MTR_SCOPE_FUNC();
    std::lock_guard<std::mutex> proposal_guard(pending_proposal_mutex_);
    const FinalBlock final_block(FinalizeBlock(pending_proposal_));
    pending_proposal_.setNull();
    return final_block;
  }

  /**
   *  Create a new FinalBlock based on a remotely serialized FinalBlock
   *  @param serial - a binary representation of the FinalBlock
   *  @param prior - the chain state prior to this new FinalBlock
   *  @param keys - the directory of Addresses and EC keys
   *  @return a FinalBlock based on the remote data provided
   */
  const FinalBlock FinalizeRemoteBlock(const std::vector<byte>& serial
      , const ChainState prior, const KeyRing& keys) {
    LOG_DEBUG << "FinalizeRemoteBlock()";
    MTR_SCOPE_FUNC();
    FinalBlock final(serial, prior, keys, tcm_);
    return final;
  }

  /** Remove unreferenced Transactions from the pool.
   *  @return the number of Transactions removed.
   */
  int GarbageCollect() {
    LOG_DEBUG << "GarbageCollect()";
    //TODO: delete old unrecorded Transactions periodically
    return 0;
  }

  /**
   *  @return the elapsed time since this pool last received a Transaction
   */
  double getElapsedTime() {
    return timer_.elapsed();
  }
  /**
   *  @return the tool to create Transactions in parallel
   */
  TransactionCreationManager& get_transaction_creation_manager() {
    return(tcm_);
  }

 private:
  TxMap txs_;
  mutable std::mutex txs_mutex_;

  ProposedBlock pending_proposal_;
  mutable std::mutex pending_proposal_mutex_;

  // Total number of transactions that have been
  // added to the transaction map
  size_t num_cum_txs_ = 0;
  size_t max_tx_per_block_ = 10000;

  // Time since starting
  Timer timer_;
  std::unique_ptr<MTRScopedTrace> trace_;

  TransactionCreationManager tcm_;
  eAppMode mode_;

  /**
   *  Compose transition states into a map of SmartCoin transitions by Address
   *  @param first - one set of transitions, the merge target
   *  @param second - another set of transitions, to merge into first
   *  @return a combined set of SmartCoin transitions mapped to a set of Addresses
   */
  std::map<Address, SmartCoin> MergeStates(std::map<Address, SmartCoin>& first
      , const std::map<Address, SmartCoin>& second) {
    for (const auto& item : second) {
      auto loc = first.find(item.first);
      if (loc == first.end()) {
        std::pair<Address, SmartCoin> pair(item.first, item.second);
        first.insert(pair);
      } else {
        //loc->second.amount_ += item.second.amount_;
        loc->second.setAmount(loc->second.getAmount() + item.second.getAmount());
      }
    }
    return first;
  }

  /** Verifies Transactions for this pool.
   *  @note this implementation is greedy in selecting Transactions
   *  @params state the chain state to validate against
   *  @params keys a KeyRing that provides keys for signature verification
   *  @params summary the Summary to update
   *  @return a vector of unrecorded valid transactions
   */
  std::vector<TransactionPtr> CollectValidTransactions(ChainState& state
      , const KeyRing& keys, Summary& summary) {
    LOG_DEBUG << "CollectValidTransactions()";
    std::vector<TransactionPtr> valid;
    MTR_SCOPE_FUNC();
    std::lock_guard<std::mutex> guard(txs_mutex_);
    unsigned int num_txs = 0;
    std::map<Address, SmartCoin> aggregate;
    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter) {
      /*aggregate = iter->second.second->aggregateState(aggregate
                                        , state, keys, summary);*/
      if (iter->second.second->isValid(state, keys, summary)) {
        valid.push_back(std::move(iter->second.second->clone()));
        iter->second.first++;
        num_txs++;
        if (num_txs >= max_tx_per_block_) break;
      }
    }
    /*state.addCoins(aggregate);
    for (const auto& item : aggregate) {
      if (!summary.addItem(item.first, item.second.coin_, item.second.amount_)) {
        LOG_FATAL << "An aggregated transaction was invalid!!";
        valid.clear();
      }
    }*/
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
    LOG_DEBUG << "ReverifyTransactions()";
    MTR_SCOPE_FUNC();
    std::lock_guard<std::mutex> guard(txs_mutex_);
    for (auto const& item : txs) {
      if (!item.isValid(state, keys, summary)) {
        return false;
      }
    }
    return true;
  }

  /** Removes Transactions in a ProposedBlock from this pool
   *  @param proposed - the ProposedBlock containing Transactions to remove
   *  @return true iff, all transactions in the block were removed
   *  @return false otherwise
   */
  bool RemoveTransactions(const ProposedBlock& proposed) {
    std::lock_guard<std::mutex> guard(txs_mutex_);
    size_t txs_size = txs_.size();
    for (auto const& item : proposed.getTransactions()) {
      if (txs_.erase(item->getSignature()) == 0) {
        LOG_WARNING << "RemoveTransactions(): ret = 0, transaction not found: "
                    << ToHex(item->getSignature());
      } else {
        LOG_TRACE << "RemoveTransactions(): erase returned 1: " << ToHex(item->getSignature());
      }
    }
    LOG_DEBUG << "RemoveTransactions: (to remove/size pre/size post) ("
              << proposed.getNumTransactions() << "/"
              << txs_size << "/"
              << txs_.size() << ")";
    return true;
  }

  /** Finalize a ProposedBlock
   *  @param proposed - the ProposedBlock to finalize
   *  @return the FinalBlock based on the provided ProposedBlock
   */
  const FinalBlock FinalizeBlock(const ProposedBlock& proposal) {
    LOG_DEBUG << "FinalizeBlock()";
    MTR_SCOPE_FUNC();
    RemoveTransactions(proposal);
    FinalBlock final(proposal);
    return final;
  }

};

} //end namespace Devcash


#endif /* CONSENSUS_UNRECORDEDTRANSACTIONPOOL_H_ */
