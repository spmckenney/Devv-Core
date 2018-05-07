/*
 * Tier1Transaction.h defines the structure of the transaction section of a block
 * for Tier 1 shards.
 *
 *  Created on: May 4, 2018
 *  Author: Nick Williams
 *
 */

#ifndef PRIMITIVES_TIER1TRANSACTION_H_
#define PRIMITIVES_TIER1TRANSACTION_H_

#include <string>
#include <vector>
#include <stdint.h>

#include "Transfer.h"
#include "Summary.h"
#include "Validation.h"
#include "consensus/KeyRing.h"
#include "consensus/chainstate.h"

using namespace Devcash;

namespace Devcash
{

class Tier1Transaction : public Transaction {
 public:

/** Constructors */
  explicit Tier1Transaction(const std::vector<byte>& serial, const KeyRing& keys)
    : sum_size_(0)
  {
    MTR_SCOPE_FUNC();
    int trace_int = 124;
    MTR_START("Transaction", "Transaction", &trace_int);
    MTR_STEP("Transaction", "Transaction", &trace_int, "step1");

    canonical_.insert(canonical_.end(), serial.begin()
            , serial.begin());
    size_t offset = 8;

    canonical_.insert(canonical_.end(), serial.begin()+offset
            , serial.begin()+8);
    sum_size_ = BinToUint64(serial, offset);
    offset += 8;

    MTR_STEP("Transaction", "Transaction", &trace_int, "step2");
    if (serial.size() < offset+sum_size_+kSIG_SIZE) {
      LOG_WARNING << "Invalid serialized T1 transaction, too small!";
      return;
    }

    canonical_.insert(canonical_.end(), serial.begin()+offset
            , serial.begin()+sum_size_+kSIG_SIZE);
    offset += sum_size_+kSIG_SIZE;

    MTR_STEP("Transaction", "Transaction", &trace_int, "sound");
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
    }
    MTR_FINISH("Transaction", "Transaction", &trace_int);
  }

  /**
   * Constructor
   *
   * @params serial
   * @params offset
   * @params keys a KeyRing that provides keys for signature verification
   */
  explicit Tier1Transaction(const std::vector<byte>& serial, size_t& offset
    , const KeyRing& keys, bool calculate_soundness = true) : sum_size_(0)
  {
    MTR_SCOPE_FUNC();
    int trace_int = 124;
    MTR_START("Transaction", "Transaction", &trace_int);
    MTR_STEP("Transaction", "Transaction", &trace_int, "step1");

    canonical_.insert(canonical_.end(), serial.begin()+offset
            , serial.begin()+8);
    offset += 8;

    canonical_.insert(canonical_.end(), serial.begin()+offset
            , serial.begin()+8);
    sum_size_ = BinToUint64(serial, offset);
    offset += 8;

    MTR_STEP("Transaction", "Transaction", &trace_int, "step2");
    if (serial.size() < 16+sum_size_+kSIG_SIZE) {
      LOG_WARNING << "Invalid serialized T1 transaction, too small!";
      return;
    }

    canonical_.insert(canonical_.end(), serial.begin()+offset
            , serial.begin()+sum_size_+kSIG_SIZE);
    offset += sum_size_+kSIG_SIZE;

    MTR_STEP("Transaction", "Transaction", &trace_int, "sound");
    if (calculate_soundness) {
      is_sound_ = isSound(keys);
      if (!is_sound_) {
        LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
      }
    }
    MTR_FINISH("Transaction", "Transaction", &trace_int);
  }

  Tier1Transaction(const Summary& summary, const Signature& sig, uint64_t nonce,
      const KeyRing& keys)
      : Transaction(0, false), sum_size_(summary.getByteSize()) {
    if (!summary.isSane()) {
      LOG_WARNING << "Serialized T1 transaction has bad summary!";
    }
    xfer_count_ = summary.getTransferCount();
    canonical_.reserve(sum_size_+16+kSIG_SIZE);
    Uint64ToBin(nonce, canonical_);
    Uint64ToBin(sum_size_, canonical_);
    std::vector<byte> sum_canon(summary.getCanonical());
    canonical_.insert(std::end(canonical_), std::begin(sum_canon), std::end(sum_canon));
    canonical_.insert(std::end(canonical_), std::begin(sig), std::end(sig));
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
    }
  }

  std::vector<byte> getMessageDigest() const {
    std::vector<byte> md(canonical_.begin()
        , canonical_.begin()+sum_size_+8);
    return md;
  }

 private:
  uint64_t sum_size_;

  byte do_getOperation() const {
    return (byte) 0;
  }

  std::vector<Transfer> do_getTransfers() const {
    size_t offset = 16;
    Summary summary(canonical_, offset);
    return summary.getTransfers();
  }

  uint64_t do_getNonce() const {
    return BinToUint64(canonical_, 0);
  }

  Signature do_getSignature() const {
    Signature sig;
    std::copy_n(canonical_.begin()+sum_size_+16
        , kSIG_SIZE, sig.begin());
    return sig;
  }

  bool do_setIsSound(const KeyRing& keys)
  {
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
    }
    return is_sound_;
  }

  /** Checks if this transaction is sound, meaning potentially valid.
   *  If any portion of the transaction is invalid,
   *  the entire transaction is also unsound.
   * @params keys a KeyRing that provides keys for signature verification
   * @return true iff the transaction is sound
   * @return false otherwise
   */
    bool do_isSound(const KeyRing& keys) const
    {
      MTR_SCOPE_FUNC();
      CASH_TRY {
        if (is_sound_) return(is_sound_);

        int node_index = getNonce();
        EC_KEY* eckey(keys.getNodeKey(node_index));
        std::vector<byte> msg(getMessageDigest());
        Signature sig = getSignature();

        if (!VerifyByteSig(eckey, dcHash(msg), sig)) {
          LOG_WARNING << "Error: T1 transaction signature did not validate.\n";
          LOG_DEBUG << "Transaction state is: "+getJSON();
          LOG_DEBUG << "Node index is: "
            +std::to_string(node_index);
          LOG_DEBUG << "Signature is: "
            +toHex(std::vector<byte>(std::begin(sig), std::end(sig)));
          return false;
        }
        return true;
      } CASH_CATCH (const std::exception& e) {
        LOG_WARNING << FormatException(&e, "transaction");
      }
      return false;
    }

/** Checks if this transaction is valid with respect to a chain state.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 * @params state the chain state to validate against
 * @params keys a KeyRing that provides keys for signature verification
 * @params summary the Summary to update
 * @return true iff the transaction is valid
 * @return false otherwise
 */
  bool do_isValid(ChainState& state, const KeyRing& keys, Summary& summary) const
  {
    CASH_TRY {
      if (!isSound(keys)) return false;

      std::vector<Transfer> xfers = getTransfers();
      for (auto it = xfers.begin(); it != xfers.end(); ++it) {
        int64_t amount = it->getAmount();
        uint64_t coin = it->getCoin();
        Address addr = it->getAddress();
        if (amount < 0) {
          if (!keys.isINN(addr)) {
            LOG_WARNING << "Coins not available at addr.";
            return false;
          }
        }
        SmartCoin next_flow(addr, coin, amount);
        state.addCoin(next_flow);
        summary.addItem(addr, coin, amount, it->getDelay());
      }
      return true;
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "transaction");
    }
    return false;
  }

/** Returns a JSON string representing this transaction.
 * @return a JSON string representing this transaction.
*/
  std::string do_getJSON() const {
    std::string json("{\""+kXFER_COUNT_TAG+"\":");
    json += std::to_string(xfer_count_)+",";
    json += "\""+kOPER_TAG+"\":"+std::to_string(getOperation())+",";
    json += "\""+kXFER_TAG+"\":[";
    bool isFirst = true;
    std::vector<Transfer> xfers = getTransfers();
    for (auto it = xfers.begin(); it != xfers.end(); ++it) {
      if (isFirst) {
        isFirst = false;
      } else {
        json += ",";
      }
      json += it->getJSON();
    }
    json += "],\""+kNONCE_TAG+"\":"+std::to_string(getNonce())+",";
    Signature sig = getSignature();
    json += "\""+kSIG_TAG+"\":\""
      +toHex(std::vector<byte>(std::begin(sig), std::end(sig)))+"\"}";
    return json;
  }
};

} //end namespace Devcash

#endif /* PRIMITIVES_TIER1TRANSACTION_H_ */
