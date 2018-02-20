/*
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 *  Distributed under the MIT software license, see the accompanying
 *  file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "block.h"
#include <chrono>

#include "../common/json.hpp"
#include "../common/ossladapter.h"
#include "../common/util.h"
using json = nlohmann::json;

//exception toggling capability
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && not defined(DEVCASH_NOEXCEPTION)
    #define CASH_THROW(exception) throw exception
    #define CASH_TRY try
    #define CASH_CATCH(exception) catch(exception)
#else
    #define CASH_THROW(exception) std::abort()
    #define CASH_TRY if(true)
    #define CASH_CATCH(exception) if(false)
#endif

std::string DCBlockHeader::GetHash() const
{
    return this->hashMerkleRoot;
}

std::string DCBlockHeader::ToJSON() const {
  json out = json();
  out["version"] = this->nVersion;
  out["prevHash"] = this->hashPrevBlock;
  out["hashMerkleRoot"] = this->hashMerkleRoot;
  out["time"] = this->nTime;
  out["bytes"] = this->nBytes;
  out["txBytes"] = this->txSize;
  out["vBytes"] = this->vSize;
  return(out.dump());
}

DCBlock::DCBlock(std::vector<DCTransaction> &txs, DCValidationBlock &validations)
  : vtx(txs), vals(validations)
{
  CASH_TRY {
  fChecked = false;
  DCBlockHeader();
  } CASH_CATCH (const std::exception* e) {
  FormatException(e, "block init", DCLog::ALL);
  }
}

bool DCBlock::validate(EC_KEY* eckey) {
  LogPrintStr("Begin block validation.");
  for (auto iter = vtx.begin(); iter != vtx.end();) {
  DCTransaction tx = *iter;
  if (!tx.isValid(eckey)) {
  LogPrintStr("Remove invalid transaction: "+tx.ToJSON());
  iter = vtx.erase(iter);
  } else {
  iter++;
  }
  }
  LogPrintStr("Block validation completed.");
  if (vtx.size() < 1) return(false);
  return(true);
}

bool DCBlock::signBlock(EC_KEY* eckey) {
  LogPrintStr("Sign block.");
  std::string txBlock("");
  for (auto iter = vtx.begin(); iter != vtx.end(); ++iter) {
  txBlock += iter->ToJSON();
  }
  //TODO: hash before signing?
  //char hashMem[SHA256_DIGEST_LENGTH*2+1];
  //dcHash(txBlock, *hashMem);
  std::string sDer = sign(eckey, txBlock);
  vals.addValidation(std::string("toy/miner"), std::string(sDer));
  this->nBytes=txBlock.size();
  LogPrintStr("Finished signing block.");
  return(true);
}

bool DCBlock::finalize() {
  LogPrintStr("Finalize block.");
  std::string txHashes("");
  for (std::vector<DCTransaction>::iterator iter = vtx.begin(); iter != vtx.end(); ++iter) {
  DCTransaction tx = *iter;
  txHashes += tx.ComputeHash();
  }
  std::string txSubHash(strHash(txHashes));
  std::string valHashes(vals.GetHash());
  this->hashPrevBlock="Genesis";  //TODO: check for previous block Merkle
  this->hashMerkleRoot=strHash(txSubHash+strHash(valHashes));

  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
  uint64_t now = ms.count();
  this->nTime=now;
  this->txSize=vtx.size();
  this->vSize=vals.GetValidationCount();
  LogPrintStr("Block finalized.");
  return(true);
}

std::string DCBlock::ToJSON() const
{
    json j = {{"hashPrevBlock", hashPrevBlock},
      {"hashMerkleRoot", hashMerkleRoot},
  {"nBytes", nBytes},
      {"nTime", nTime},
      {"txSize", txSize},
      {"vSize", vSize}};
    json txs = json::array();
  for (std::vector<DCTransaction>::iterator iter = vtx.begin(); iter != vtx.end(); ++iter) {
  txs += json::parse(iter->ToJSON());
  }
  j += {"txs", txs};
  j += {"vals", json::parse(vals.ToJSON())};
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


