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

#include "../common/json.hpp"
#include "../common/ossladapter.h"
using json = nlohmann::json;

namespace Devcash
{

static const std::string kTX_TAG = "txs";

enum eOpType : char {
  Create     = 0x00,
  Modify     = 0x01,
  Exchange   = 0x10,
  Delete     = 0x11};

class DCTransfer {
 public:

  std::string addr_;
  long amount_;
  uint32_t nonce_;
  std::string sig_;

/** Sets this to an empty transfer. */
  void SetNull() { addr_ = ""; amount_ = 0; }

/** Checks if this is an empty transfer.
 *  @return true iff this transfer is empty and invalid
*/
  bool IsNull() const { return ("" == addr_); }

/** Constructors */
  DCTransfer();
  explicit DCTransfer(std::string json);
  explicit DCTransfer(std::vector<uint8_t> cbor);
  explicit DCTransfer(const DCTransfer &other);

/** Gets this transfer in a canonical form.
 * @return a string defining this transaction in canonical form.
 */
  std::string getCanonical();

/** Compare transfers */
  friend bool operator==(const DCTransfer& a, const DCTransfer& b)
  {
      return (a.addr_ == b.addr_ && a.amount_ == b.amount_ && a.nonce_ == b.nonce_);
  }

  friend bool operator!=(const DCTransfer& a, const DCTransfer& b)
  {
    return !(a.addr_ == b.addr_ && a.amount_ == b.amount_ && a.nonce_ == b.nonce_);
  }

/** Assign transfers */
  DCTransfer* operator=(DCTransfer&& other)
  {
    if (this != &other) {
    this->addr_ = other.addr_;
    this->amount_ = other.amount_;
    this->nonce_ = other.nonce_;
    this->sig_ = other.sig_;
    }
    return this;
  }
  DCTransfer* operator=(const DCTransfer& other)
  {
    if (this != &other) {
    this->addr_ = other.addr_;
    this->amount_ = other.amount_;
    this->nonce_ = other.nonce_;
    this->sig_ = other.sig_;
    }
    return this;
  }
};

class DCTransaction {
 public:

  eOpType oper_;
  std::string type_;
  std::vector<DCTransfer>* xfers_;

/** Constructors */
  DCTransaction();
  explicit DCTransaction(std::string jsonTx);
  explicit DCTransaction(json jsonObj);
  explicit DCTransaction(std::vector<uint8_t> cbor);
  DCTransaction(const DCTransaction& tx);

/** Checks if this transaction is valid.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 * @params ecKey the public key to use for signature verification
 * @return true iff the transaction is valid
 * @return false otherwise
 */
  bool isValid(EC_KEY* ecKey) const;
  std::string ComputeHash() const;

/** Returns the hash of this transaction.
 * @return the hash of this transaction if it is valid
 * @return an arbitrary string otherwise, empty string by default
*/
  const std::string& GetHash() const {
      return hash_;
  }

/** Returns the quantity of coins affected by this transaction.
 * @return the quantity of coins affected by this transaction.
*/
  long GetValueOut() const;

/** Returns the transaction size in bytes.
 * @return the transaction size in bytes.
*/
  unsigned int GetByteSize() const;

/** Comparison Operators */
  friend bool operator==(const DCTransaction& a, const DCTransaction& b)
  {
    return a.hash_ == b.hash_;
  }

  friend bool operator!=(const DCTransaction& a, const DCTransaction& b)
  {
    return a.hash_ != b.hash_;
  }

/** Assignment Operators */
  DCTransaction* operator=(DCTransaction&& other)
  {
    if (this != &other) {
      this->hash_ = other.hash_;
      this->jsonStr_ = other.jsonStr_;
      this->oper_ = other.oper_;
      this->type_ = other.type_;
      this->xfers_ = std::move(other.xfers_);
    }
    return this;
  }

  DCTransaction* operator=(const DCTransaction& other)
  {
    if (this != &other) {
      this->hash_ = other.hash_;
      this->jsonStr_ = other.jsonStr_;
      this->oper_ = other.oper_;
      this->type_ = other.type_;
      this->xfers_ = std::move(other.xfers_);
    }
    return this;
  }

/** Returns a JSON string representing this transaction.
 * @return a JSON string representing this transaction.
*/
  std::string ToJSON() const;

/** Returns a CBOR byte vector representing this transaction.
 * @return a CBOR byte vector representing this transaction.
*/
  std::vector<uint8_t> ToCBOR() const;


 private:

  std::string jsonStr_ = "";
  std::string hash_ = "";

/** Returns a canonical string representation of this transaction.
 * @return a canonical string representation of this transaction.
*/
  std::string GetCanonical() const;
};

} //end namespace Devcash

#endif // DEVCASH_PRIMITIVES_TRANSACTION_H

