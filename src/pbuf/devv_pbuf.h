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
    throw std::runtime_error("Sender key not set (key == null)");
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
    throw std::runtime_error("Sender key not set (key == null)");
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

std::vector<TransactionPtr> DecomposeProposal(const Devv::proto::Proposal& proposal
                             , const Blockchain& chain) {
	std::vector<TransactionPtr> ptrs;
    std::string oracle_name = proposal.oraclename();
    if (oracle_name == api::getOracleName()) {
      api oracle(proposal.data());
      std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
      ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin())
                            , std::make_move_iterator(actions.end()));
    } else if (oracle_name == data::getOracleName()) {
      data oracle(proposal.data());
      std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
      ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin())
                            , std::make_move_iterator(actions.end()));
    } else if (oracle_name == dcash::getOracleName()) {
      dcash oracle(proposal.data());
      std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
      ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin())
                            , std::make_move_iterator(actions.end()));
    } else if (oracle_name == dnero::getOracleName()) {
      dnero oracle(proposal.data());
      std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
      ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin())
                            , std::make_move_iterator(actions.end()));
    } else if (oracle_name == dneroavailable::getOracleName()) {
      dneroavailable oracle(proposal.data());
      std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
      ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin())
                            , std::make_move_iterator(actions.end()));
    } else if (oracle_name == dnerowallet::getOracleName()) {
      dnerowallet oracle(proposal.data());
      std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
      ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin())
                            , std::make_move_iterator(actions.end()));
    } else if (oracle_name == id::getOracleName()) {
      id oracle(proposal.data());
      std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
      ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin())
                            , std::make_move_iterator(actions.end()));
    } else if (oracle_name == vote::getOracleName()) {
      vote oracle(proposal.data());
      std::vector<TransactionPtr> actions = validateOracle(oracle, chain);
      ptrs.insert(ptrs.end(), std::make_move_iterator(actions.begin())
                            , std::make_move_iterator(actions.end()));
    } else {
      LOG_ERROR << "Unknown oracle: "+oracle_name;
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

  return(t2tx_ptr);
}

} // namespace Devcash

#endif //DEVCASH_DEVV_PBUF_H
