/*
 * block.h implements the structure of a Devcash block.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 *  Distributed under the MIT software license, see the accompanying
 *  file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "block.h"

#include <algorithm>
#include <chrono>

#include "transaction.h"
#include "common/json.hpp"
#include "common/logger.h"
#include "common/ossladapter.h"
#include "consensus/KeyRing.h"

using json = nlohmann::json;

namespace Devcash
{

using namespace Devcash;

static const std::string kVERSION_TAG = "v";
static const std::string kPREV_HASH_TAG = "prev";
static const std::string kMERKLE_TAG = "merkle";
static const std::string kBYTES_TAG = "bytes";
static const std::string kTIME_TAG = "time";
static const std::string kTX_SIZE_TAG = "txlen";
static const std::string kSUM_SIZE_TAG = "sumlen";
static const std::string kVAL_SIZE_TAG = "vlen";
static const std::string kTXS_TAG = "txs";
static const std::string kSUM_TAG = "sum";
static const std::string kVAL_TAG = "vals";

DCBlock::DCBlock()
{
}

//note the order of elements is assumed, which is fast, but not proper JSON
DCBlock::DCBlock(std::string rawBlock)
{
  CASH_TRY {
    if (rawBlock.at(0) == '{') {
      size_t pos = 0;
      hashPrevBlock_ =jsonFinder(rawBlock, kPREV_HASH_TAG, pos);
      hashMerkleRoot_=jsonFinder(rawBlock, kMERKLE_TAG, pos);
      nBytes_=std::stoul(jsonFinder(rawBlock, kBYTES_TAG, pos));
      nTime_=std::stoul(jsonFinder(rawBlock, kTIME_TAG, pos));
      txSize_=std::stoul(jsonFinder(rawBlock, kTX_SIZE_TAG, pos));
      sumSize_=std::stoul(jsonFinder(rawBlock, kSUM_SIZE_TAG, pos));
      vSize_=std::stoul(jsonFinder(rawBlock, kVAL_SIZE_TAG, pos));

      size_t dex = rawBlock.find("\""+kTXS_TAG+"\":[", pos);
      dex += kTXS_TAG.size()+4;
      size_t eDex = rawBlock.find(kSIG_TAG, dex);
      eDex = rawBlock.find("}", eDex);
      std::string oneTx = rawBlock.substr(dex, eDex-dex);
      vtx_.push_back(DCTransaction(oneTx));
      while (rawBlock.at(eDex+1) != ']' && eDex < rawBlock.size()-2) {
        pos = eDex;
        dex = rawBlock.find("{", pos);
        dex++;
        eDex = rawBlock.find(kSIG_TAG, dex);
        eDex = rawBlock.find("}", eDex);
        oneTx = rawBlock.substr(dex, eDex-dex);
        vtx_.push_back(DCTransaction(oneTx));
      }

      std::string valSection = rawBlock.substr(eDex+2);
      vals_ = *(new DCValidationBlock(valSection));
    } else {
      LOG_WARNING << "Invalid block input:"+rawBlock+"\n----------------\n";
    }
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

DCBlock::DCBlock(std::vector<DCTransaction>& txs
    , DCValidationBlock& validations)
  : vtx_(txs), vals_(validations), vSize_(0), sumSize_(0), txSize_(0)
  , nTime_(0), nBytes_(0)
{
}

DCBlock::DCBlock(const DCBlock& other)
  : vtx_(other.vtx_), vals_(other.vals_), vSize_(0), sumSize_(0), txSize_(0)
  , nTime_(0), nBytes_(0) {
}

bool DCBlock::validate(DCState& chainState, KeyRing& keys) {
  for (auto iter = vtx_.begin(); iter != vtx_.end(); ++iter) {
    if (!iter->isValid(chainState, keys, vals_.summaryObj_)) {
      LOG_WARNING << "Invalid transaction: "+iter->getCanonicalForm()+"\n";
      //iter = vtx_.erase(iter);
      return false;
    }
  }
  if (vtx_.size() < 1) return false;

  if (vals_.summaryObj_.isSane()) {
    LOG_WARNING << "Summary is invalid!\n";
    return false;
  }

  std::string md(vals_.summaryObj_.toCanonical());
  int i=0;
  for (auto iter = vals_.sigs_.begin(); iter != vals_.sigs_.end();) {
    if(!verifySig(keys.getKey(iter->first), md, iter->second)) {
      LOG_WARNING << "Invalid transaction: \n";
    }
  }

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
  nBytes_=txBlock.size();
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
  hashPrevBlock_="Genesis";  //TODO: check for previous block Merkle
  hashMerkleRoot_=strHash(txSubHash+strHash(valHashes));

  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>
    (std::chrono::system_clock::now().time_since_epoch());
  uint64_t now = ms.count();
  nTime_=now;
  txSize_=vtx_.size();
  vSize_=vals_.GetValidationCount();
  return(true);
}

std::string DCBlock::ToJSON() const
{
  std::string out = "{\""+kPREV_HASH_TAG+"\":\""+hashPrevBlock_+"\",";
  out += "\""+kMERKLE_TAG+"\":\""+hashMerkleRoot_+"\",";
  out += "\""+kBYTES_TAG+"\":"+std::to_string(nBytes_)+",";
  out += "\""+kTIME_TAG+"\":"+std::to_string(nTime_)+",";
  out += "\""+kTX_SIZE_TAG+"\":"+std::to_string(txSize_)+",";
  out += "\""+kSUM_SIZE_TAG+"\":"+std::to_string(sumSize_)+",";
  out += "\""+kVAL_SIZE_TAG+"\":"+std::to_string(vSize_)+",";
  out += "\""+kTXS_TAG+"\":[";
  bool isFirst = true;
  for (auto iter = vtx_.begin(); iter != vtx_.end(); ++iter) {
      if (!isFirst) {
        out += ",";
      }
      isFirst = false;
      out += iter->ToJSON();
  }
  out += "],";
  out += vals_.ToJSON();
  return(out);
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
