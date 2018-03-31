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
  :  vSize_(0)
  , sumSize_(0)
  , txSize_(0)
  , nTime_(0)
  , nBytes_(0)
{
}

//note the order of elements is assumed, which is fast, but not proper JSON
DCBlock::DCBlock(std::string rawBlock, KeyRing& keys)
  :  vSize_(0)
  , sumSize_(0)
  , txSize_(0)
  , nTime_(0)
  , nBytes_(0)
{
  CASH_TRY {
    if (rawBlock.at(0) == '{') {
      size_t pos = 0;
      hashPrevBlock_ =jsonFinder(rawBlock, kPREV_HASH_TAG, pos);
      LOG_DEBUG << "Previous: "+hashPrevBlock_;
      hashMerkleRoot_=jsonFinder(rawBlock, kMERKLE_TAG, pos);
      LOG_DEBUG << "Merkle: "+hashMerkleRoot_;
      nBytes_=std::stoul(jsonFinder(rawBlock, kBYTES_TAG, pos));
      LOG_DEBUG << "Bytes: "+std::to_string(nBytes_);
      nTime_=std::stoul(jsonFinder(rawBlock, kTIME_TAG, pos));
      LOG_DEBUG << "Time: "+std::to_string(nTime_);
      txSize_=std::stoul(jsonFinder(rawBlock, kTX_SIZE_TAG, pos));
      LOG_DEBUG << "TxSize: "+std::to_string(txSize_);
      sumSize_=std::stoul(jsonFinder(rawBlock, kSUM_SIZE_TAG, pos));
      LOG_DEBUG << "SumSize: "+std::to_string(sumSize_);
      vSize_=std::stoul(jsonFinder(rawBlock, kVAL_SIZE_TAG, pos));
      LOG_DEBUG << "vSize: "+std::to_string(vSize_);

      size_t dex = rawBlock.find("\""+kTXS_TAG+"\":[", pos);
      dex += kTXS_TAG.size()+4;
      size_t eDex = rawBlock.find(kSIG_TAG, dex);
      eDex = rawBlock.find("}", eDex);
      std::string oneTx = rawBlock.substr(dex, eDex-dex+1);
      LOG_DEBUG << "One transaction: "+oneTx;
      DCTransaction new_tx(oneTx);
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
      LOG_DEBUG << "Parse validation section: "+valSection;
      vals_ = *(new DCValidationBlock(valSection));

      LOG_DEBUG << std::to_string(vtx_.size())+" new transactions.";

      for (std::vector<DCTransaction>::iterator iter = vtx_.begin();
          iter != vtx_.end(); ++iter) {
        if (!iter->isValid(block_state_, keys, vals_.summaryObj_)) {
          LOG_WARNING << "Invalid transaction:"+iter->ToJSON();
        }
      }
    } else {
      LOG_WARNING << "Invalid block input:"+rawBlock+"\n----------------\n";
    }
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "transaction");
  }
}

DCBlock::DCBlock(std::vector<DCTransaction>& txs,
                 DCValidationBlock& validations)
  : vtx_(txs)
  , vals_(validations)
  , vSize_(0)
  , sumSize_(0)
  , txSize_(0)
  , nTime_(0)
  , nBytes_(0)
{
}

DCBlock::DCBlock(const DCBlock& other)
  : vtx_(other.vtx_)
  , vals_(other.vals_)
  , vSize_(0)
  , sumSize_(0)
  , txSize_(0)
  , nTime_(0)
  , nBytes_(0) {
}

bool DCBlock::setBlockState(const DCState& prior_state) {
  block_state_.stateMap_ = prior_state.stateMap_;
  return true;
}

bool DCBlock::validate(KeyRing& keys) {
  if (vtx_.size() < 1) {
    LOG_WARNING << "Trying to validate empty block.";
    return false;
  }

  if (!vals_.summaryObj_.isSane()) {
    LOG_WARNING << "Summary is invalid in block.validate()!\n";
    LOG_DEBUG << "Summary state: "+vals_.summaryObj_.toCanonical();
    return false;
  }

  std::string md = vals_.summaryObj_.toCanonical();
  LOG_DEBUG << "Block digest: "+md;
  for (auto iter = vals_.sigs_.begin(); iter != vals_.sigs_.end(); ++iter) {
    if(!verifySig(keys.getKey(iter->first), strHash(md), iter->second)) {
      LOG_WARNING << "Invalid block signature";
      LOG_DEBUG << "Block Node Addr: "+iter->first;
      LOG_DEBUG << "Block Node Sig: "+iter->second;
      return false;
    }
  }

  return true;
}

bool DCBlock::addTransaction(std::string txStr, KeyRing&) {
  CASH_TRY {
    DCTransaction new_tx(txStr);
    long nValueOut = 0;

    if (new_tx.nonce_ < 1) {
      LOG_WARNING << "Error: nonce is required";
      return(false);
    }

    std::string senderAddr;

    for (auto it = new_tx.xfers_.begin(); it != new_tx.xfers_.end(); ++it) {
      nValueOut += it->amount_;
        if (it->amount_ < 0) {
          if (!senderAddr.empty()) {
            LOG_WARNING << "Multiple senders in transaction!";
            return false;
          }
          senderAddr = it->addr_;
        }
        SmartCoin next_flow(it->coinIndex_, it->addr_, it->amount_);
        block_state_.addCoin(next_flow);
        vals_.summaryObj_.addItem(it->addr_, it->coinIndex_, it->amount_, it->delay_);
    }
    if (nValueOut != 0) {
      LOG_WARNING << "Error: transaction amounts are asymmetric. (sum="+std::to_string(nValueOut)+")";
      return false;
    }
    vtx_.push_back(new_tx);
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "transaction");
  }
  return false;
}

bool DCBlock::signBlock(EC_KEY* eckey, std::string myAddr) {
  std::string sumBlock = vals_.summaryObj_.toCanonical();
  LOG_DEBUG << "Validator signs: "+sumBlock;
  std::string hash(strHash(sumBlock));
  std::string sDer = sign(eckey, hash);
  vals_.addValidation(myAddr, sDer);
  return true;
}

bool DCBlock::finalize(std::string prevHash) {
  std::string txHashes("");
  uint32_t tx_size = 0;
  uint32_t sum_bytes = vals_.summaryObj_.getByteSize();
  uint32_t v_bytes = vals_.GetByteSize();
  for (std::vector<DCTransaction>::iterator iter = vtx_.begin();
    iter != vtx_.end(); ++iter) {
      DCTransaction tx = *iter;
      tx_size += tx.ToJSON().size();
      txHashes += tx.ComputeHash();
  }
  std::string txSubHash(strHash(txHashes));
  std::string valHashes(vals_.GetHash());
  hashPrevBlock_=prevHash;
  hashMerkleRoot_=strHash(txSubHash+strHash(valHashes));

  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>
    (std::chrono::system_clock::now().time_since_epoch());
  uint64_t now = ms.count();
  nTime_=now;
  txSize_=tx_size;
  sumSize_= sum_bytes;
  vSize_=v_bytes;
  nBytes_=tx_size+sum_bytes+v_bytes;
  return true;
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
  out += vals_.ToJSON()+"}";
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
