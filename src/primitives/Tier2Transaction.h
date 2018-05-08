/*
 * Tier2Transaction.h defines the structure of the transaction section of a block
 * for Tier 2 shards.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 *
 *
 */

#ifndef PRIMITIVES_TIER2TRANSACTION_H_
#define PRIMITIVES_TIER2TRANSACTION_H_

#include "Transaction.h"

using namespace Devcash;

namespace Devcash
{

 class Tier2Transaction : public Transaction {
 public:

/** Constructors */
  Tier2Transaction(const std::vector<byte>& serial, const KeyRing& keys) {
    if (serial.size() < MinSize()) {
      LOG_WARNING << "Invalid serialized T2 transaction, too small!";
      return;
    }
    xfer_count_ = BinToUint64(serial, 0);
    size_t tx_size = MinSize()+(Transfer::Size()*xfer_count_);
    LOG_INFO << "TX size: "+std::to_string(tx_size);
    if (serial.size() < tx_size) {
      LOG_WARNING << "Invalid serialized T2 transaction, wrong size!";
      LOG_WARNING << "Transaction prefix: "+toHex(serial);
      return;
    }
    LOG_INFO << "TX canonical: "+toHex(serial);
    canonical_.insert(canonical_.end(), serial.begin()
        , serial.begin()+tx_size);
    if (getOperation() > 3) {
      LOG_WARNING << "Invalid serialized T2 transaction, invalid operation!";
      return;
    }
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T2 transaction, not sound!";
    }
  }

  /**
   * Constructor
   *
   * @params serial
   * @params offset
   * @params keys a KeyRing that provides keys for signature verification
   */
  explicit Tier2Transaction(const std::vector<byte>& serial, size_t& offset
    , const KeyRing& keys, bool calculate_soundness = true)
  {
    MTR_SCOPE_FUNC();
    int trace_int = 124;
    MTR_START("Transaction", "Transaction", &trace_int);
    MTR_STEP("Transaction", "Transaction", &trace_int, "step1");
    if (serial.size() < offset+MinSize()) {
      LOG_WARNING << "Invalid serialized T2 transaction, too small!";
      return;
    }
    xfer_count_ = BinToUint64(serial, offset);
    size_t tx_size = MinSize()+(Transfer::Size()*xfer_count_);
    if (serial.size() < offset+tx_size) {
      std::vector<byte> prefix(serial.begin()+offset, serial.begin()+offset+8);
      LOG_WARNING << "Invalid serialized T2 transaction, wrong size ("
          +std::to_string(tx_size)+")!";
      LOG_WARNING << "Transaction prefix: "+toHex(prefix);
      LOG_WARNING << "Bytes offset: "+std::to_string(offset);
      return;
    }
    MTR_STEP("Transaction", "Transaction", &trace_int, "step2");
    canonical_.insert(canonical_.end(), serial.begin()+offset
        , serial.begin()+(offset+tx_size));
    offset += tx_size;
    if (getOperation() > 3) {
      LOG_WARNING << "Invalid serialized T2 transaction, invalid operation!";
      return;
    }
    MTR_STEP("Transaction", "Transaction", &trace_int, "sound");
    if (calculate_soundness) {
      is_sound_ = isSound(keys);
      if (!is_sound_) {
        LOG_WARNING << "Invalid serialized T2 transaction, not sound!";
      }
    }
    MTR_FINISH("Transaction", "Transaction", &trace_int);
  }

  Tier2Transaction(uint64_t xfer_count, byte oper
      , const std::vector<Transfer>& xfers, uint64_t nonce, Signature sig
      , const KeyRing& keys)
      : Transaction(xfer_count, false) {
    canonical_.reserve(MinSize()+(Transfer::Size()*xfer_count_));

    Uint64ToBin(xfer_count_, canonical_);
    canonical_.push_back(oper);
    for (auto it = xfers.begin(); it != xfers.end(); ++it) {
      std::vector<byte> xfer_canon(it->getCanonical());
      canonical_.insert(std::end(canonical_), std::begin(xfer_canon)
        , std::end(xfer_canon));
    }
    Uint64ToBin(nonce, canonical_);
    canonical_.insert(std::end(canonical_), std::begin(sig), std::end(sig));
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T2 transaction, not sound!";
    }
  }

  Tier2Transaction(byte oper, const std::vector<Transfer>& xfers, uint64_t nonce
      , EC_KEY* eckey, const KeyRing& keys)
      : Transaction(xfers.size(), false) {
    canonical_.reserve(MinSize()+(Transfer::Size()*xfer_count_));

    Uint64ToBin(xfer_count_, canonical_);
    canonical_.push_back(oper);
    for (auto it = xfers.begin(); it != xfers.end(); ++it) {
      std::vector<byte> xfer_canon(it->getCanonical());
      canonical_.insert(std::end(canonical_), std::begin(xfer_canon)
        , std::end(xfer_canon));
    }
    Uint64ToBin(nonce, canonical_);
    std::vector<byte> msg(getMessageDigest());
    Signature sig;
    SignBinary(eckey, dcHash(msg), sig);
    canonical_.insert(std::end(canonical_), std::begin(sig), std::end(sig));
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized T2 transaction, not sound!";
    }
  }

  std::unique_ptr<Transaction> Clone() const override {
    return std::unique_ptr<Transaction>(new Tier2Transaction(*this));
  }

 private:

  byte do_getOperation() const {
    return canonical_[8];
  }

  std::vector<Transfer> do_getTransfers() const {
    std::vector<Transfer> out;
    for (size_t i=0; i<xfer_count_; ++i) {
      size_t offset = 9+(Transfer::Size()*i);
      Transfer* t = new Transfer(canonical_, offset);
      out.push_back(*t);
    }
    return out;
  }

  uint64_t do_getNonce() const {
    return BinToUint64(canonical_, 9+(Transfer::Size()*xfer_count_));
  }

  Signature do_getSignature() const {
    Signature sig;
    std::copy_n(canonical_.begin()+(17+(Transfer::Size()*xfer_count_))
        , kSIG_SIZE, sig.begin());
    return sig;
  }

  bool do_setIsSound(const KeyRing& keys)
  {
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized transaction, not sound!";
    }
    return is_sound_;
  }

  bool do_isSound(const KeyRing& keys) const
  {
    MTR_SCOPE_FUNC();
    CASH_TRY {
      if (is_sound_) return(is_sound_);
      long total = 0;
      byte oper = getOperation();

      if (getNonce() < 1) {
        LOG_WARNING << "Error: nonce is required";
        return(false);
      }

      bool sender_set = false;
      Address sender;

      std::vector<Transfer> xfers = getTransfers();
      for (auto it = xfers.begin(); it != xfers.end(); ++it) {
        int64_t amount = it->getAmount();
        total += amount;
        if ((oper == eOpType::Delete && amount > 0) ||
          (oper != eOpType::Delete && amount < 0) || oper == eOpType::Modify) {
        }
          if (amount < 0) {
            if (sender_set) {
              LOG_WARNING << "Multiple senders in transaction!";
              return false;
            }
            sender = it->getAddress();
            sender_set = true;
          }
      }
      if (total != 0) {
        LOG_WARNING << "Error: transaction amounts are asymmetric. (sum="
            +std::to_string(total)+")";
        return false;
      }

      if ((oper != Exchange) && (!keys.isINN(sender))) {
        LOG_WARNING << "INN transaction not performed by INN!";
        return false;
      }

      EC_KEY* eckey(keys.getKey(sender));
      std::vector<byte> msg(getMessageDigest());
      Signature sig = getSignature();

      if (!VerifyByteSig(eckey, dcHash(msg), sig)) {
        LOG_WARNING << "Error: transaction signature did not validate.\n";
        LOG_DEBUG << "Transaction state is: "+getJSON();
        LOG_DEBUG << "Sender addr is: "
          +toHex(std::vector<byte>(std::begin(sender), std::end(sender)));
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

  bool do_isValid(ChainState& state, const KeyRing& keys, Summary& summary) const override
  {
    CASH_TRY {
      if (!isSound(keys)) return false;
      byte oper = getOperation();
      std::vector<Transfer> xfers = getTransfers();
      for (auto it = xfers.begin(); it != xfers.end(); ++it) {
        int64_t amount = it->getAmount();
        uint64_t coin = it->getCoin();
        Address addr = it->getAddress();
        if (amount < 0) {
          if ((oper == Exchange) && (amount > state.getAmount(coin, addr))) {
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

typedef std::unique_ptr<Tier2Transaction> Tier2TransactionPtr;

} //end namespace Devcash

#endif // DEVCASH_PRIMITIVES_TRANSACTION_H
