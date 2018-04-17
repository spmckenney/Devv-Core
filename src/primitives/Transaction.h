/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include <primitives/Transfer.h>

/**
 * Transaction
 */
template <typename StorageManager, typename StorageStrategy>
class Transaction {
public:
  // Type of this class
  typedef StorageManager<StorageStrategy>::TransactionType TransactionType;

  // Type of implementation struct
  typedef StorageManager<StorageStrategy>::TransactionTypeImpl TransactionTypeImpl;

  typedef StorageManager<StorageStrategy>::TransferType TransferType;
  typedef StorageManager<StorageStrategy>::TransferListType TransferListType;
  typedef StorageManager<StorageStrategy>::StateType StateType;

  Transaction(size_t num_transfers);

  /** Comparison Operators */
  friend bool operator==(const TransactionType& a, const TransactionType& b)
  {
    return a.hash_ == b.hash_;
  }

  friend bool operator!=(const TransactionType& a, const TransactionType& b)
  {
    return a.hash_ != b.hash_;
  }

  /** Assignment Operators */
  TransactionType* operator=(TransactionType&& other)
  {
    if (this->transaction_ != &other.transaction_) {
      this->transaction_ = other.transaction_;
      this->hash_ = other.hash_;
      this->jsonStr_ = other.jsonStr_;
      this->oper_ = other.oper_;
      this->nonce_ = other.nonce_;
      this->sig_ = other.sig_;
      this->xfers_ = std::move(other.xfers_);
    }
    return this;
  }

  TransactionType* operator=(const TransactionType& other)
  {
    if (this != &other) {
      this->hash_ = other.hash_;
      this->jsonStr_ = other.jsonStr_;
      this->oper_ = other.oper_;
      this->nonce_ = other.nonce_;
      this->sig_ = other.sig_;
      this->xfers_ = std::move(other.xfers_);
    }
    return this;
  }

  int64_t nonce() {
    return(transaction_.nonce());
  }

  void set_nonce(int64_t nonce) {
    transaction_.set_nonce(nonce);
  }

  /**
   * Get a transfer at a location
   */
  const TransferType& GetTransferAt(size_t transfer_index) const {
    transaction_.at(transfer_index);
  }

  /**
   * Set a transfer at a location
   */
  bool SetTransferAt(size_t transfer_index, const Transfer& transfer) {
    transfers_.at(transfer_index) = transfer;
  }

  /**
   * Set the nonce to nullptr
   */
  void SetNull() {
    transaction_.nonce = nullptr;
  }

  /**
   * Check if the nonce is null
   */
  bool IsNull() const {
    return(transaction_.nonce < 1);
  }

/** Checks if this transaction is valid.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 * @params ecKey the public key to use for signature verification
 * @return true iff the transaction is valid
 * @return false otherwise
 */
  bool IsValid (StateType state, const KeyRingType& keys, SummaryType summary) {
  }

  /**
   * Compute the hash of this transaction
   */
  // FIXME(spm) should this be a method?
  std::string ComputeHash() const {
  }

  const std::string& GetHash() const {
  }

  uint64_t GetValueOut() const {
  }

  uint64_t GetByteSize() const {
  }

/**
 * Returns a canonical string representation of this transaction.
 * @return a canonical string representation of this transaction.
 */
  std::string GetCanonicalForm() const {
  }

/** Checks if this transaction has a certain operation type.
 * @param string, one of "Create", "Modify", "Exchange", "Delete".
 * @return true iff the transaction is this type
 * @return false, otherwise
*/
  // FIXME(smckenney) this should be an enum
  bool IsOpType(const std::string& oper);

private:
  Transaction();

  TransactionTypeImpl transaction_;
}
