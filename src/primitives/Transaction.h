/*
 * transaction.h defines the structure of the transaction section of a block.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 *
 *  **Transaction Structure**
 *  size - xfer_count in Tier2, sum_size in Tier1
 *  oper - coin operation (one of create, modify, exchange, delete)
 *  xfer - array of participants
 *  nonce - generated by wallet
 *  sig - generated by wallet
 *
 *  xfer params:
 *  addr - wallet address
 *  type - the coin type
 *  amount - postive implies giving, negative implies taking, 0 implies neither
 *  delay - seconds until this Transfer is considered final
 *
 */

#ifndef DEVCASH_PRIMITIVES_TRANSACTION_H
#define DEVCASH_PRIMITIVES_TRANSACTION_H

#include <cstdint>
#include <string>
#include <vector>

#include "Summary.h"
#include "Transfer.h"
#include "Validation.h"
#include "primitives/buffers.h"

#include "consensus/KeyRing.h"
#include "consensus/chainstate.h"

using namespace Devcash;

namespace Devcash {

static const std::string kXFER_SIZE_TAG = "xfer_size";
static const std::string kSUMMARY_TAG = "summary";
static const std::string kSUM_SIZE_TAG = "sum_size";
static const std::string kOPER_TAG = "oper";
static const std::string kXFER_TAG = "xfer";
static const std::string kNONCE_TAG = "nonce";
static const std::string kNONCE_SIZE_TAG = "nonce_size";
static const std::string kSIG_TAG = "sig";
static const std::string kVALIDATOR_DEX_TAG = "val_dex";

/**
 * Types of operations performed by transactions
 */
enum eOpType : byte { Create = 0, Modify = 1, Exchange = 2, Delete = 3 };

/**
 * The Transaction Abstract Base Class
 * Subclassed as Tier1Transaction and Tier2Transaction
 */
class Transaction {
 public:
  /**
   * Constructor
   * @param xfer_count
   * @param is_sound
   */
  Transaction(bool is_sound) : canonical_(), is_sound_(is_sound) {}

  /**
   * Constructor
   * @param xfer_count
   * @param canonical
   * @param is_sound
   */
  Transaction(const std::vector<byte>& canonical, bool is_sound)
      : canonical_(canonical), is_sound_(is_sound) {}

  /**
   * Copy constructor
   * @param other
   */
  Transaction(const Transaction& other) = default;

  /** Destructor */
  virtual ~Transaction() = default;

  /**
   * Comparison operator
   * @param a
   * @param b
   * @return
   */
  friend bool operator==(const Transaction& a, const Transaction& b) { return a.canonical_ == b.canonical_; }

  /**
   * Comparison operator
   * @param a
   * @param b
   * @return
   */
  friend bool operator!=(const Transaction& a, const Transaction& b) { return a.canonical_ != b.canonical_; }

  /**
   * Returns minimum size (hard-coded to 89)
   * @return minimum size (hard-coded to 89)
   */
  static size_t MinSize() { return (89 + 2); }

  /**
   * Returns envelope size (hard-coded to 17)
   * @return envelope size (hard-coded to 17)
   */
  static size_t EnvelopeSize() { return 17; }

  /**
   * Returns transfer offset (hard-coded to 17)
   * @return transfer offset (hard-coded to 17)
   */
  static size_t transferOffset() { return 17; }

  /**
   * Returns min nonce size (hard-coded to 8)
   * @return min nonce size (hard-coded to 8)
   */
  static size_t minNonceSize() { return 8; }

 /**
  * Returns min nonce size (hard-coded to 8)
  * @return min nonce size (hard-coded to 8)
  */
  static size_t uint64Size() { return 8; }

  /**
   * Make a deep copy of the TierXTransaction subclass
   * @return TransactionPtr - a unique pointer to the new copy
   */
  virtual std::unique_ptr<Transaction> clone() const = 0;

  /**
   * Returns a canonical bytestring representation of this transaction.
   * @return a canonical bytestring representation of this transaction.
   */
  const std::vector<byte>& getCanonical() const { return canonical_; }

  /**
   * Returns the message digest bytestring for this transaction.
   * @return the message digest bytestring for this transaction.
   */
  std::vector<byte> getMessageDigest() const { return do_getMessageDigest(); }

  /**
   * Returns the transaction size in bytes.
   * @return the transaction size in bytes.
   */
  size_t getByteSize() const { return (canonical_.size()); }

  /**
   * Get a vector of Transfers
   * @return vector of Transfer objects
   */
  std::vector<TransferPtr> getTransfers() const { return do_getTransfers(); }

  /**
   * Get Signature
   * @return
   */
  Signature getSignature() const { return do_getSignature(); }

  /**
   * Calculate and set the soundness for this Transaction
   * @param keys
   * @return
   */
  bool setIsSound(const KeyRing& keys) { return do_setIsSound(keys); }

  /**
   * Checks if this transaction is sound, meaning potentially valid.
   * If any portion of the transaction is invalid,
   * the entire transaction is also unsound.
   * @param keys a KeyRing that provides keys for signature verification
   * @return true iff the transaction is sound, false otherwise
   */
  bool isSound(const KeyRing& keys) const { return do_isSound(keys); }

  /**
   * Checks if this transaction is valid with respect to a chain state.
   * Transactions are atomic, so if any portion of the transaction is invalid,
   * the entire transaction is also invalid.
   * @param state the chain state to validate against
   * @param keys a KeyRing that provides keys for signature verification
   * @param summary the Summary to update
   * @return true iff the transaction is valid, false otherwise
   */
  virtual bool isValid(ChainState& state, const KeyRing& keys, Summary& summary) const {
    return do_isValid(state, keys, summary);
  }

  /**
   * Checks if this transaction is valid with respect to a chain state and other transactions.
   * Transactions are atomic, so if any portion of the transaction is invalid,
   * the entire transaction is also invalid.
   * If this transaction is invalid given the transfers implied by the aggregate map,
   * then this function returns false.
   * @note if this transaction is valid based on the chainstate, aggregate is updated
   *       based on the signer of this transaction.
   * @param state the chain state to validate against
   * @param keys a KeyRing that provides keys for signature verification
   * @param summary the Summary to update
   * @param aggregate, coin sending information about parallel transactions
   * @param prior, the chainstate before adding any new transactions
   * @return true iff the transaction is valid in this context, false otherwise
   */
  virtual bool isValidInAggregate(ChainState& state, const KeyRing& keys
               , Summary& summary, std::map<Address, SmartCoin>& aggregate
               , const ChainState& prior) const {
    return do_isValidInAggregate(state, keys, summary, aggregate, prior);
  }

  /**
   * Returns a JSON string representing this transaction.
   * @return a JSON string representing this transaction.
   */
  std::string getJSON() const { return do_getJSON(); }

 protected:
  /**
   * Default Constructor
   */
  Transaction() = default;

  /// The canonical representation of this Transaction
  std::vector<byte> canonical_ = {};

  /// True if this Transaction is sound
  bool is_sound_ = false;

 private:
  virtual std::vector<byte> do_getMessageDigest() const = 0;

  virtual std::vector<TransferPtr> do_getTransfers() const = 0;

  virtual Signature do_getSignature() const = 0;

  virtual bool do_setIsSound(const KeyRing& keys) = 0;

  virtual bool do_isSound(const KeyRing& keys) const = 0;

  virtual bool do_isValid(ChainState& state, const KeyRing& keys
                         , Summary& summary) const = 0;

  virtual bool do_isValidInAggregate(ChainState& state, const KeyRing& keys
      , Summary& summary, std::map<Address, SmartCoin>& aggregate
      , const ChainState& prior) const = 0;

  virtual std::string do_getJSON() const = 0;
};

typedef std::unique_ptr<Transaction> TransactionPtr;

}  // end namespace Devcash

#endif  // DEVCASH_PRIMITIVES_TRANSACTION_H
