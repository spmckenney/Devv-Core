/*
 * vote.cpp implements an oracle to handle arbitrary election processes.
 *
 *  Created on: Feb 28, 2018
 *  Author: Nick Williams
 *
 */

#include "vote.h"

#include "common/json.hpp"

using json = nlohmann::json;

namespace Devcash
{

using namespace Devcash;

DCVote::DCVote() {
  election_ = "";
  targets_ = new std::vector<std::string>(0);
}

DCVote::DCVote(std::string election, std::vector<std::string>* targets) {
  election_ = election;
  targets_ = targets;
}

DCVote::~DCVote() {
}

bool DCVote::isValid(Devcash::DCTransaction checkTx) {
  if (checkTx.isOpType("exchange")) {
    if (targets_->empty() || election_.empty()) {
      LOG_WARNING << "Error: Voting in an uninitialized election.";
      return false;
    }
    for (std::vector<Devcash::DCTransfer>::iterator it=checkTx.xfers_.begin();
        it != checkTx.xfers_.end(); ++it) {
      if (it->amount_ > 0) {
        if (targets_->end() == std::find(targets_->begin(),
            targets_->end(), it->addr_)) {
          LOG_WARNING << "Error: Vote for unspecified candidate.";
          return false;
        } //endif sending to specified target
      } //endif receiving vote
    } //end for each transfer
  } //endif operation is exchange
  return true;
}

bool DCVote::isValid(Devcash::DCTransaction checkTx,
    Devcash::DCState& context) {
  if (checkTx.isOpType("exchange")) {
    if (targets_->empty() || election_.empty()) {
      LOG_WARNING << "Error: Voting in an uninitialized election.";
      return false;
    }
    for (std::vector<Devcash::DCTransfer>::iterator it=checkTx.xfers_.begin();
        it != checkTx.xfers_.end(); ++it) {
      if (it->amount_ > 0) {
        if (targets_->end() == std::find(targets_->begin(),
            targets_->end(), it->addr_)) {
          LOG_WARNING << "Error: Vote for unspecified candidate.";
          return false;
        } //endif sending to specified target
      } else if (it->amount_ < 0) { //sending vote
        if (context.getAmount(election_, it->addr_) < it->amount_) {
          LOG_WARNING << "Error: Voter lacks votes in this election.";
          return false;
        } //endif voter has enough votes in this election context
      } //end send vs receive vote
    } //end for each transfer
  } //endif operation is exchange
  return true;
}

DCTransaction DCVote::getT1Syntax(Devcash::DCTransaction theTx) {
  Devcash::DCTransaction out(theTx);
  DCTransfer election(election_, DCVote::getCoinIndex(), 1, 0);
  out.xfers_.push_back(election);
  return(out);
}

DCTransaction DCVote::Tier2Process(std::string rawTx,
    Devcash::DCState context) {
  json jsonObj = json::parse(rawTx);
  Devcash::DCTransaction tx(jsonObj);
  if (tx.isOpType("create") || tx.isOpType("modify")) {
    if (jsonObj["election"].empty()) {
      LOG_WARNING << "Error: No address for new election.";
      tx.setNull();
      return tx;
    }
    election_ = jsonObj["election"];
    if (!jsonObj["targets"].is_array()) {
      LOG_WARNING << "Error: No targets for new election.";
      tx.setNull();
      return tx;
    }
    targets_ = new std::vector<std::string>(0);
    for (auto iter = jsonObj["targets"].begin();
        iter != jsonObj["targets"].end(); ++iter) {
      std::string target = iter.value().dump();
      targets_->push_back(target);
    }
  } else if (tx.isOpType("exchange")) {
    if (!isValid(tx, context)) {
      tx.setNull();
      return tx;
    }
  }
  DCTransfer election(election_, DCVote::getCoinIndex(), 1, 0);
  tx.xfers_.push_back(election);
  return tx;
}

} //end namespace Devcash
