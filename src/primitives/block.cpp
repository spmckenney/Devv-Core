/*
 * block.h implements the structure of a Devcash block.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 *  Distributed under the MIT software license, see the accompanying
 *  file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "block.h"

#include <chrono>

#include "common/json.hpp"
#include "common/logger.h"
#include "common/ossladapter.h"

using json = nlohmann::json;

namespace Devcash
{

using namespace Devcash;

std::string DCBlockHeader::GetHash() const
{
  return this->hashMerkleRoot_;
}

std::string DCBlockHeader::ToJSON() const {
  json out = json();
  out["version"] = this->nVersion_;
  out["prevHash"] = this->hashPrevBlock_;
  out["hashMerkleRoot"] = this->hashMerkleRoot_;
  out["time"] = this->nTime_;
  out["bytes"] = this->nBytes_;
  out["txBytes"] = this->txSize_;
  out["vBytes"] = this->vSize_;
  return(out.dump());
}

DCBlock::DCBlock(std::vector<DCTransaction> &txs, DCValidationBlock &validations)
  : vtx_(txs), vals_(validations)
{
  //CASH_TRY {
    DCBlockHeader();
  /*} CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "block");
  }*/
}

bool DCBlock::validate(EC_KEY* eckey) {
  for (auto iter = vtx_.begin(); iter != vtx_.end();) {
    DCTransaction tx = *iter;
    if (!tx.isValid(eckey)) {
      LOG_WARNING << "Remove invalid transaction: "+tx.ToJSON();
     iter = vtx_.erase(iter);
    } else {
      iter++;
    }
  }
  if (vtx_.size() < 1) return false;
  return true;
}

bool DCBlock::signBlock(EC_KEY* eckey) {
  std::string txBlock("");
  for (auto iter = vtx_.begin(); iter != vtx_.end(); ++iter) {
    txBlock += iter->ToJSON();
  }
  std::string hash(strHash(txBlock));
  std::string sDer = sign(eckey, hash);
  vals_.addValidation(std::string("toy/miner"), std::string(sDer));
  this->nBytes_=txBlock.size();
  return(true);
}

bool DCBlock::finalize() {
  std::string txHashes("");
  for (std::vector<DCTransaction>::iterator iter = vtx_.begin();
    iter != vtx_.end(); ++iter) {
      DCTransaction tx = *iter;
      txHashes += tx.ComputeHash();
  }
  std::string txSubHash(strHash(txHashes));
  std::string valHashes(vals_.GetHash());
  this->hashPrevBlock_="Genesis";  //TODO: check for previous block Merkle
  this->hashMerkleRoot_=strHash(txSubHash+strHash(valHashes));

  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>
    (std::chrono::system_clock::now().time_since_epoch());
  uint64_t now = ms.count();
  this->nTime_=now;
  this->txSize_=vtx_.size();
  this->vSize_=vals_.GetValidationCount();
  return(true);
}

std::string DCBlock::ToJSON() const
{
  json j = {{"hashPrevBlock", hashPrevBlock_},
    {"hashMerkleRoot", hashMerkleRoot_},
    {"nBytes", nBytes_},
    {"nTime", nTime_},
    {"txSize", txSize_},
    {"vSize", vSize_}};
  json txs = json::array();
  for (std::vector<DCTransaction>::iterator iter = vtx_.begin();
    iter != vtx_.end(); ++iter) {
      txs += json::parse(iter->ToJSON());
  }
  j += {"txs", txs};
  j += {"vals", json::parse(vals_.ToJSON())};
  return(j.dump());
}

std::vector<uint8_t> DCBlock::ToCBOR() const
{
  return json::to_cbor(ToJSON());
}

std::string DCBlock::ToCBOR_str() {
  std::vector<uint8_t> b = ToCBOR();
  int len = b.size();
  std::stringstream ss;
  for (int j=0; j<len; j++) {
    int c = (int) b[j];
    ss.put(alpha[(c>>4)&0xF]);
    ss.put(alpha[c&0xF]);
  }
  return(ss.str());
}

} //end namespace Devcash
