/*
 * ${Filename}
 *
 *  Created on: 8/6/18
 *      Author: mckenney
 */
#ifndef DEVCASH_DEVV_PBUF_H
#define DEVCASH_DEVV_PBUF_H

#include <vector>
#include <exception>
#include <typeinfo>

#include "consensus/blockchain.h"
#include "primitives/Tier2Transaction.h"
#include "oracles/api.h"
#include "oracles/data.h"
#include "oracles/dcash.h"
#include "oracles/dnero.h"
#include "oracles/dneroavailable.h"
#include "oracles/dnerowallet.h"
#include "oracles/id.h"
#include "oracles/vote.h"

#include "devv.pb.h"

namespace Devcash {

struct Proposal {
  std::string oracle_name;
  std::string data;
};
typedef std::unique_ptr<Proposal> ProposalPtr;

struct Envelope {
  std::vector<Tier2TransactionPtr> transactions;
  std::vector<ProposalPtr> proposals;
};
typedef std::unique_ptr<Envelope> EnvelopePtr;

struct PendingTransaction {
  Signature sig;
  uint32_t expect_block = 0;
  uint32_t shard_index = 0;
};
typedef std::unique_ptr<PendingTransaction> PendingTransactionPtr;

struct AnnouncerResponse {
  uint32_t return_code = 0;
  std::string message;
  std::vector<PendingTransactionPtr> pending;
};
typedef std::unique_ptr<AnnouncerResponse> AnnouncerResponsePtr;

struct RepeaterRequest {
  int64_t timestamp = 0;
  uint32_t operation = 0;
  std::string uri;
};
typedef std::unique_ptr<RepeaterRequest> RepeaterRequestPtr;

struct RepeaterResponse {
  int64_t request_timestamp = 0;
  uint32_t operation = 0;
  uint32_t return_code = 0;
  std::string message;
  std::vector<byte> raw_response;
};
typedef std::unique_ptr<RepeaterResponse> RepeaterResponsePtr;

TransactionPtr CreateTransaction(const Devv::proto::Transaction& transaction, const KeyRing& keys, bool do_sign = false) {
  auto operation = transaction.operation();
  auto pb_xfers = transaction.xfers();

  std::vector<Transfer> transfers;
  EC_KEY* key = nullptr;
  for (auto const& xfer : pb_xfers) {
    std::vector<byte> bytes(xfer.address().begin(), xfer.address().end());
    auto address = Address(bytes);
    transfers.emplace_back(address, xfer.coin(), xfer.amount(), xfer.delay());
    if (xfer.amount() < 0) {
      if (key != nullptr) {
        throw std::runtime_error("More than one send transfer not supported.");
      }
      key = keys.getKey(address);
    }
  }

  if (key == nullptr) {
    throw std::runtime_error("Exchange transactions must have one transfer with a negative amount.");
  }

  std::vector<byte> nonce(transaction.nonce().begin(), transaction.nonce().end());

  if (do_sign) {
    Tier2Transaction t2tx(
        operation,
        transfers,
        nonce,
        key,
        keys);
    return t2tx.clone();
  } else {
    std::vector<byte> sig(transaction.sig().begin(), transaction.sig().end());
    Signature signature(sig);

    Tier2Transaction t2tx(
        operation,
        transfers,
        nonce,
        key,
        keys,
        signature);
    return t2tx.clone();
  }
}

Tier2TransactionPtr CreateTransaction(const Devv::proto::Transaction& transaction
    , std::string pk, std::string pk_pass) {
  auto operation = transaction.operation();
  auto pb_xfers = transaction.xfers();

  std::vector<Transfer> transfers;
  EC_KEY* key = nullptr;
  std::string pub_key;
  for (auto const& xfer : pb_xfers) {
    std::vector<byte> bytes(xfer.address().begin(), xfer.address().end());
    auto address = Address(bytes);
    transfers.emplace_back(address, xfer.coin(), xfer.amount(), xfer.delay());
    if (xfer.amount() < 0) {
      if (!pub_key.empty()) {
        throw std::runtime_error("More than one send transfer not supported.");
      }
      key = LoadEcKey(pub_key, pk, pk_pass);
    }
  }

  if (key == nullptr) {
    throw std::runtime_error("Exchange transactions must have one transfer with a negative amount.");
  }

  std::vector<byte> nonce(transaction.nonce().begin(), transaction.nonce().end());

  Tier2TransactionPtr t2tx_ptr = std::make_unique<Tier2Transaction>(
        operation,
        transfers,
        nonce,
        key);

  return t2tx_ptr;
}

std::vector<TransactionPtr> validateOracle(oracleInterface& oracle
                                              , const Blockchain& chain) {
  std::vector<TransactionPtr> out;
  if (oracle.isValid(chain)) {
    std::map<uint64_t, std::vector<Tier2Transaction>> oracle_actions = oracle.getTransactions(chain);
    for (auto& it : oracle_actions) {
      //TODO (nick) forward transactions for other shards to those shards
      for (auto& tx : it.second) {
        TransactionPtr t2tx_ptr = tx.clone();
        out.push_back(std::move(t2tx_ptr));
      }
    }
  }
  return out;
}

std::string SignProposal(const Devv::proto::Proposal& proposal
          , std::string addr , std::string pk, std::string pk_pass) {
  std::string oracle_name = proposal.oraclename();
  if (oracle_name == api::getOracleName()) {
    api oracle(proposal.data());
    return oracle.Sign(addr, pk, pk_pass);
  } else if (oracle_name == data::getOracleName()) {
    data oracle(proposal.data());
    return oracle.Sign(addr, pk, pk_pass);
  } else if (oracle_name == dcash::getOracleName()) {
    dcash oracle(proposal.data());
    return oracle.Sign(addr, pk, pk_pass);
  } else if (oracle_name == dnero::getOracleName()) {
    dnero oracle(proposal.data());
    return oracle.Sign(addr, pk, pk_pass);
  } else if (oracle_name == dneroavailable::getOracleName()) {
    dneroavailable oracle(proposal.data());
    return oracle.Sign(addr, pk, pk_pass);
  } else if (oracle_name == dnerowallet::getOracleName()) {
    dnerowallet oracle(proposal.data());
    return oracle.Sign(addr, pk, pk_pass);
  } else if (oracle_name == id::getOracleName()) {
    id oracle(proposal.data());
    return oracle.Sign(addr, pk, pk_pass);
  } else if (oracle_name == vote::getOracleName()) {
    vote oracle(proposal.data());
    return oracle.Sign(addr, pk, pk_pass);
  } else {
    LOG_ERROR << "Unknown oracle: "+oracle_name;
  }
  return "";
}

std::vector<TransactionPtr> DecomposeProposal(const Devv::proto::Proposal& proposal, const Blockchain& chain) {
  std::vector<TransactionPtr> ptrs;
  std::string oracle_name = proposal.oraclename();
  if (oracle_name == api::getOracleName()) {
    api oracle(proposal.data());
    std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
    ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin()), std::make_move_iterator(actions.end()));
  } else if (oracle_name == data::getOracleName()) {
    data oracle(proposal.data());
    std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
    ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin()), std::make_move_iterator(actions.end()));
  } else if (oracle_name == dcash::getOracleName()) {
    dcash oracle(proposal.data());
    std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
    ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin()), std::make_move_iterator(actions.end()));
  } else if (oracle_name == dnero::getOracleName()) {
    dnero oracle(proposal.data());
    std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
    ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin()), std::make_move_iterator(actions.end()));
  } else if (oracle_name == dneroavailable::getOracleName()) {
    dneroavailable oracle(proposal.data());
    std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
    ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin()), std::make_move_iterator(actions.end()));
  } else if (oracle_name == dnerowallet::getOracleName()) {
    dnerowallet oracle(proposal.data());
    std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
    ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin()), std::make_move_iterator(actions.end()));
  } else if (oracle_name == id::getOracleName()) {
    id oracle(proposal.data());
    std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
    ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin()), std::make_move_iterator(actions.end()));
  } else if (oracle_name == vote::getOracleName()) {
    vote oracle(proposal.data());
    std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
    ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin()), std::make_move_iterator(actions.end()));
  } else {
    LOG_ERROR << "Unknown oracle: " + oracle_name;
  }
  return ptrs;
}

std::vector<TransactionPtr> DeserializeEnvelopeProtobufString(const std::string& pb_envelope, const KeyRing& keys) {
  Devv::proto::Envelope envelope;
  envelope.ParseFromString(pb_envelope);

  std::vector<TransactionPtr> ptrs;

  auto pb_transactions = envelope.txs();
  for (auto const& transaction : pb_transactions) {
    ptrs.push_back(CreateTransaction(transaction, keys));
  }

  //TODO (nick): use the latest blockchain of this shard from the repeater
  Blockchain chain("test-shard");
  auto pb_proposals = envelope.proposals();
  for (auto const& proposal : pb_proposals) {
    std::vector<TransactionPtr> actions = DecomposeProposal(proposal, chain);
    ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin())
                          , std::make_move_iterator(actions.end()));
  }

  return ptrs;
}

TransactionPtr DeserializeTxProtobufString(const std::string& pb_tx, const KeyRing& keys, bool do_sign = false) {

  Devv::proto::Transaction tx;
  tx.ParseFromString(pb_tx);

  auto t2tx_ptr = CreateTransaction(tx, keys, do_sign);

  return t2tx_ptr;
}

Devv::proto::AnnouncerResponse SerializeAnnouncerResponse(const AnnouncerResponsePtr& response_ptr) {
  Devv::proto::AnnouncerResponse response;
  response.set_return_code(response_ptr->return_code);
  response.set_message(response_ptr->message);
  for (auto const& pending_ptr : response_ptr->pending) {
    Devv::proto::PendingTransaction* one_pending_tx = response.add_txs();
    std::string raw_sig(std::begin(pending_ptr->sig.getCanonical())
                      , std::end(pending_ptr->sig.getCanonical()));
    one_pending_tx->set_signature(raw_sig);
    one_pending_tx->set_expect_block(pending_ptr->expect_block);
    one_pending_tx->set_shard_index(pending_ptr->shard_index);
  }
  return response;
}

RepeaterRequestPtr DeserializeRepeaterRequest(const std::string& pb_request) {
  Devv::proto::RepeaterRequest incoming_request;
  incoming_request.ParseFromString(pb_request);

  RepeaterRequest request;
  request.timestamp = incoming_request.timestamp();
  request.operation = incoming_request.operation();
  request.uri = incoming_request.uri();
  return std::make_unique<RepeaterRequest>(request);
}

Devv::proto::RepeaterResponse SerializeRepeaterResponse(const RepeaterResponsePtr& response_ptr) {
  Devv::proto::RepeaterResponse response;
  response.set_request_timestamp(response_ptr->request_timestamp);
  response.set_operation(response_ptr->operation);
  response.set_return_code(response_ptr->return_code);
  response.set_message(response_ptr->message);
  std::string raw_str(std::begin(response_ptr->raw_response)
                    , std::end(response_ptr->raw_response));
  response.set_raw_response(raw_str);
  return response;
}

Devv::proto::Transaction SerializeTransaction(const Tier2Transaction& one_tx) {
  Devv::proto::Transaction tx;
  tx.set_operation(static_cast<Devv::proto::eOpType>(one_tx.getOperation()));
  std::vector<byte> nonce = one_tx.getNonce();
  std::string nonce_str(std::begin(nonce), std::end(nonce));
  tx.set_nonce(nonce_str);
  for (auto const& xfer : one_tx.getTransfers()) {
    Devv::proto::Transfer* transfer = tx.add_xfers();
    std::string addr(std::begin(xfer->getAddress().getCanonical())
                    ,std::end(xfer->getAddress().getCanonical()));
    transfer->set_address(addr);
    transfer->set_coin(xfer->getCoin());
    transfer->set_amount(xfer->getAmount());
    transfer->set_delay(xfer->getDelay());
  }
  Signature sig = one_tx.getSignature();
  std::string raw_sig(std::begin(sig.getCanonical())
                    , std::end(sig.getCanonical()));
  tx.set_sig(raw_sig);
  return tx;
}

Devv::proto::FinalBlock SerializeFinalBlock(const FinalBlock& block) {
  Devv::proto::FinalBlock final_block;
  final_block.set_version(block.getVersion());
  final_block.set_num_bytes(block.getNumBytes());
  final_block.set_block_time(block.getBlockTime());
  std::string prev_hash_str(std::begin(block.getPreviousHash()), std::end(block.getPreviousHash()));
  final_block.set_prev_hash(prev_hash_str);
  std::string merkle_str(std::begin(block.getMerkleRoot()), std::end(block.getMerkleRoot()));
  final_block.set_merkle_root(merkle_str);
  final_block.set_tx_size(block.getSizeofTransactions());
  final_block.set_sum_size(block.getSummarySize());
  final_block.set_val_count(block.getNumValidations());
  for (auto const& one_tx : block.getTransactions()) {
    Devv::proto::Transaction* tx = final_block.add_txs();
    tx = SerializeTransaction(std::move(one_tx));
  }
  std::vector<byte> summary(block.getSummary().getCanonical());
  std::string summary_str(std::begin(summary), std::end(summary));
  final_block.set_summary(summary_str);
  std::vector<byte> vals(block.getValidation().getCanonical());
  std::string vals_str(std::begin(vals), std::end(vals));
  final_block.set_vals(vals_str);
  return final_block;
}

} // namespace Devcash

#endif //DEVCASH_DEVV_PBUF_H
