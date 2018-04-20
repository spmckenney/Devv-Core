/*
 * transaction.h defines the structure of the transaction section of a block.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 *
 *  **Transaction Structure**
 *  oper - coin operation (one of create, modify, exchange, delete)
 *  type - the coin type
 *  xfer - array of participants
 *
 *  xfer params:
 *  addr - wallet address
 *  amount - postive implies giving, negative implies taking, 0 implies neither
 *  nonce - generated by wallet (required iff amount positive)
 *  sig - generated by wallet (required iff amount positive)
 *
 *
 */

#ifndef DEVCASH_PRIMITIVES_TRANSACTION_H
#define DEVCASH_PRIMITIVES_TRANSACTION_H

#include <string>
#include <vector>
#include <stdint.h>

#include "Transfer.h"
#include "Summary.h"
#include "consensus/KeyRing.h"
#include "consensus/chainstate.h"
#include "Validation.h"

using namespace Devcash;

namespace Devcash
{
static const std::string kXFER_COUNT_TAG = "xfer_count";
static const std::string kOPER_TAG = "oper";
static const std::string kXFER_TAG = "xfer";
static const std::string kNONCE_TAG = "nonce";
static const std::string kSIG_TAG = "sig";

enum eOpType : byte {
  Create     = 0,
  Modify     = 1,
  Exchange   = 2,
  Delete     = 3};

class Transaction {
 public:
  uint64_t xfer_count_;
  byte oper_;
  std::vector<Transfer> xfers_;
  uint64_t nonce_;
  Signature sig_;

/** Constructors */
  Transaction() : xfer_count_(0), oper_(eOpType::Create), nonce_(0), sig_()  {}
  explicit Transaction(const std::vector<byte>& serial)
    : xfer_count_(0), oper_(0), nonce_(0), sig_() {
    if (serial.size() < MinSize()) {
      LOG_WARNING << "Invalid serialized transaction, too small!";
      return;
    }
    xfer_count_ = BinToUint64(serial, 0);
    if (serial.size() < MinSize()+(Transfer::Size()*xfer_count_)) {
      LOG_WARNING << "Invalid serialized transaction, wrong size!";
    }
    oper_ = serial[8];
    if (oper_ > 3) {
      LOG_WARNING << "Invalid serialized transaction, invalid operation!";
      return;
    }
    for (size_t i=0; i<xfer_count_; ++i) {
      Transfer* t = new Transfer(serial, 9+(Transfer::Size()*i));
      xfers_.push_back(*t);
    }
    size_t offset = 9+(Transfer::Size()*xfer_count_);
    nonce_ = BinToUint64(serial, offset);
    offset += 8;
    std::copy_n(serial.begin()+offset, kSIG_SIZE, sig_.begin());
  }

  explicit Transaction(const std::vector<byte>& serial, size_t pos)
    : xfer_count_(0), oper_(0), nonce_(0), sig_() {
    if (serial.size() < pos+MinSize()) {
      LOG_WARNING << "Invalid serialized transaction, too small!";
      return;
    }
    xfer_count_ = BinToUint64(serial, pos);
    if (serial.size() < pos+MinSize()+(Transfer::Size()*xfer_count_)) {
      LOG_WARNING << "Invalid serialized transaction, wrong size!";
    }
    oper_ = serial[pos+8];
    if (oper_ > 3) {
      LOG_WARNING << "Invalid serialized transaction, invalid operation!";
      return;
    }
    for (size_t i=0; i<xfer_count_; ++i) {
      Transfer* t = new Transfer(serial, pos+9+(Transfer::Size()*i));
      xfers_.push_back(*t);
    }
    size_t offset = pos+9+(Transfer::Size()*xfer_count_);
    nonce_ = BinToUint64(serial, offset);
    offset += 8;
    std::copy_n(serial.begin()+offset, kSIG_SIZE, sig_.begin());
  }

  Transaction(uint64_t xfer_count, byte oper
      , const std::vector<Transfer>& xfers, uint64_t nonce, Signature sig)
      : xfer_count_(xfer_count), oper_(oper), xfers_(), nonce_(nonce)
      , sig_(sig) {
    for (int i=0; i<xfer_count_; ++i) {
      xfers_.push_back(xfers.at(i));
    }
  }

  Transaction(const Transaction& other) : xfer_count_(other.xfer_count_)
    , oper_(other.oper_), nonce_(other.nonce_), sig_(other.sig_) {
  }

  Transaction(byte oper, const std::vector<Transfer>& xfers, uint64_t nonce
      , EC_KEY* eckey)
      : xfer_count_(xfers.size()), oper_(oper), xfers_(), nonce_(nonce)
      , sig_() {
    for (size_t i=0; i<xfer_count_; ++i) {
      xfers_.push_back(xfers.at(i));
    }

    std::vector<byte> msg(getMessageDigest());
    SignBinary(eckey, dcHash(msg), sig_);
    LOG_INFO << "New sig: "+toHex(std::vector<byte>(std::begin(sig_), std::end(sig_)));
  }

  static const size_t MinSize() {
    return 89;
  }

  static const size_t EnvelopeSize() {
    return 17;
  }

  static const size_t MessageDigestEnvelopeSize() {
    return 9;
  }

  /** Comparison Operators */
  friend bool operator==(const Transaction& a, const Transaction& b)
  {
    return a.sig_ == b.sig_;
  }

  friend bool operator!=(const Transaction& a, const Transaction& b)
  {
    return a.sig_ != b.sig_;
  }

  /** Assignment Operators */
  Transaction* operator=(Transaction&& other)
  {
    if (this != &other) {
      this->oper_ = other.oper_;
      this->nonce_ = other.nonce_;
      this->sig_ = other.sig_;
      this->xfers_ = std::move(other.xfers_);
    }
    return this;
  }

  Transaction* operator=(const Transaction& other)
  {
    if (this != &other) {
      this->oper_ = other.oper_;
      this->nonce_ = other.nonce_;
      this->sig_ = other.sig_;
      this->xfers_ = std::move(other.xfers_);
    }
    return this;
  }

/** Checks if this transaction is valid.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 * @params state the chain state to validate against
 * @params ecKey the public key to use for signature verification
 * @return true iff the transaction is valid
 * @return false otherwise
 */
  bool isValid(DCState& state, const KeyRing& keys, Summary& summary) const
  {
    CASH_TRY {
        long total = 0;

        if (nonce_ < 1) {
          LOG_WARNING << "Error: nonce is required";
          return(false);
        }

        bool sender_set = false;
        Address sender;

        for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
          total += it->amount_;
          if ((oper_ == eOpType::Delete && it->amount_ > 0) ||
            (oper_ != eOpType::Delete && it->amount_ < 0) || oper_ == eOpType::Modify) {
            if (it->delay_ < 0) {
              LOG_WARNING << "Error: A negative delay is not allowed.";
              return false;
            }
          }
            if (it->amount_ < 0) {
              if (sender_set) {
                LOG_WARNING << "Multiple senders in transaction!";
                return false;
              }
              if ((oper_ == Exchange) && (it->amount_ > state.getAmount(it->coin_, it->addr_))) {
                LOG_WARNING << "Coins not available at addr.";
                return false;
              }
              std::copy_n(it->addr_.begin(), kADDR_SIZE, sender.begin());
              sender_set = true;
            }
            SmartCoin next_flow(it->addr_, it->coin_, it->amount_);
            state.addCoin(next_flow);
            summary.addItem(it->addr_, it->coin_, it->amount_, it->delay_);
        }
        if (total != 0) {
          LOG_WARNING << "Error: transaction amounts are asymmetric. (sum="+std::to_string(total)+")";
          return false;
        }

        if ((oper_ != Exchange) && (!keys.isINN(sender))) {
          LOG_WARNING << "INN transaction not performed by INN!";
          return false;
        }

        EC_KEY* eckey(keys.getKey(sender));
        std::vector<byte> msg(getMessageDigest());

        if (!VerifyByteSig(eckey, dcHash(msg), sig_)) {
          LOG_WARNING << "Error: transaction signature did not validate.\n";
          LOG_DEBUG << "Transaction state is: "+getJSON();
          LOG_DEBUG << "Sender addr is: "
            +toHex(std::vector<byte>(std::begin(sender), std::end(sender)));
          LOG_DEBUG << "Signature is: "
            +toHex(std::vector<byte>(std::begin(sig_), std::end(sig_)));
          return false;
        }
        return true;
      } CASH_CATCH (const std::exception& e) {
        LOG_WARNING << FormatException(&e, "transaction");
      }
      return false;
  }

/** Returns a canonical bytestring representation of this transaction.
 * @return a canonical bytestring representation of this transaction.
*/
  std::vector<byte> getCanonical() const {
    std::vector<byte> serial;
    serial.reserve(MinSize()
        +(Transfer::Size()*xfer_count_));

    Uint64ToBin(xfer_count_, serial);
    serial.push_back(oper_);
    for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
      std::vector<byte> xfer_canon(it->getCanonical());
      serial.insert(std::end(serial), std::begin(xfer_canon), std::end(xfer_canon));
    }
    Uint64ToBin(nonce_, serial);
    serial.insert(std::end(serial), std::begin(sig_), std::end(sig_));
    return serial;
  }

  /** Returns the message digest bytestring for this transaction.
   * @return the message digest bytestring for this transaction.
  */
  std::vector<byte> getMessageDigest() const {
    std::vector<byte> md;
    md.reserve(MessageDigestEnvelopeSize()
        +(Transfer::Size()*xfer_count_));

    md.push_back(oper_);
    for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
      std::vector<byte> xfer_canon(it->getCanonical());
      md.insert(std::end(md), std::begin(xfer_canon), std::end(xfer_canon));
    }
    Uint64ToBin(nonce_, md);
    return md;
  }

  /** Returns the transaction size in bytes.
   * @return the transaction size in bytes.
  */
  size_t getByteSize() const {
    return getCanonical().size();
  }

/** Returns a JSON string representing this transaction.
 * @return a JSON string representing this transaction.
*/
  std::string getJSON() const {
    std::string json("{\""+kXFER_COUNT_TAG+"\":");
    json += std::to_string(xfer_count_)+",";
    json += "\""+kOPER_TAG+"\":"+std::to_string(oper_)+",";
    json += "\""+kXFER_TAG+"\":[";
    bool isFirst = true;
    for (auto it = xfers_.begin(); it != xfers_.end(); ++it) {
      if (isFirst) {
        isFirst = false;
      } else {
        json += ",";
      }
      json += it->getJSON();
    }
    json += "],\""+kNONCE_TAG+"\":"+std::to_string(nonce_)+",";
    json += "\""+kSIG_TAG+"\":\""
      +toHex(std::vector<byte>(std::begin(sig_), std::end(sig_)))+"\"}";
    return json;
  }

/** Returns a CBOR byte vector representing this transaction.
 * @return a CBOR byte vector representing this transaction.
*/
  //std::vector<uint8_t> ToCBOR() const;

};

} //end namespace Devcash

#endif // DEVCASH_PRIMITIVES_TRANSACTION_H
