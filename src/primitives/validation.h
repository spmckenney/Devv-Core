/*
 * validation.h defines the structure of the validation section of a block.
 *
 *  Created on: Jan 3, 2018
 *  Author: Nick Williams
 *
 *  **Validation Structure**
 *  The validation block has two parts.
 *  1. A summary of the change in chain state caused by the transactions.
 *  2. A map of miner addresses to signatures.
 *
 *  **Core Validation Logic**
 *  All of the following statements must be true;
 *  1. The sum of all xfer.amounts in a transaction must be 0.
 *  2. Every xfer.sig must validate against its xfer.addr.
 *  3. In 'exchange' operations, for each sender, xfer.amount must exactly equal the current sum of its onchain coins.
 *  4. In 'create','modify','delete' operations, each sender addr must be part of the INN.
 *
 */

#ifndef SRC_PRIMITIVES_VALIDATION_H_
#define SRC_PRIMITIVES_VALIDATION_H_

#include <stdint.h>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <boost/container/flat_map.hpp>

namespace Devcash
{

struct DCSummaryItem {
  long delta=0;
  long delay=0;
  DCSummaryItem(long delta=0, long delay=0) : delta(delta), delay(delay) {}
};

typedef std::unordered_map<std::string, std::string> vmap;
typedef std::unordered_map<long, DCSummaryItem> coinmap;

class DCSummary {
 public:
  /** Constrcutors */
  DCSummary();
  DCSummary(std::string canonical);
  DCSummary(const DCSummary& other);

  /** Assignment Operators */
  DCSummary* operator=(DCSummary&& other)
  {
    if (this != &other) {
      this->summary_ = std::move(other.summary_);
    }
    return this;
  }
  DCSummary* operator=(DCSummary& other)
  {
    if (this != &other) {
      this->summary_ = std::move(other.summary_);
    }
    return this;
  }

  /** Adds a summary record to this block.
   *  @param addr the address that this change applies to
   *  @param item a chain state vector summary of transactions.
   *  @return true iff the summary was added
  */
  bool addItem(std::string addr, long coinType, DCSummaryItem item);
  /** Adds a summary record to this block.
   *  @param addr the addresses involved in this change
   *  @param coinType the type number for this coin
   *  @param delta change in coins (>0 for receiving, <0 for spending)
   *  @param delay the delay in seconds before this transaction can be received
   *  @return true iff the summary was added
  */
  bool addItem(std::string addr, long coinType, long delta, long delay=0);

  /** Adds multiple summary records to this block.
   *  @param addr the addresses involved in these changes
   *  @param items map of DCSummary items detailing these changes by coinType
   *  @return true iff the summary was added
  */
  bool addItems(std::string addr,
      std::unordered_map<long, DCSummaryItem> items);

  /**
   *  @return a canonical string summarizing these changes.
  */
  std::string toCanonical();

  size_t getByteSize();

  /**
   *  @return true iff, the summary passes sanity checks
  */
  bool isSane();

  typedef boost::container::flat_map<std::string, coinmap> SummaryMap;
  SummaryMap summary_;
  std::mutex lock_;
};

class DCValidationBlock {
 public:
  vmap sigs_;
  DCSummary summaryObj_;

/** Checks if this is an empty validation block.
 *  @return true iff this validation block is empty and invalid
*/
  bool IsNull() const { return (sigs_.size() < 1); }

/** Constrcutors */
  DCValidationBlock();
  DCValidationBlock(std::vector<uint8_t> cbor);
  DCValidationBlock(std::string& jsonMsg);
  DCValidationBlock(const DCValidationBlock& other);
  DCValidationBlock(std::string node, std::string sig);
  DCValidationBlock(std::unordered_map<std::string, std::string> sigs);

/** Adds a validation record to this block.
 *  @param node a string containing the URI of node that produced this validation
 *  @param sig the signature of the validating node
 *  @return true iff the validation verified against the node's public key
*/
  bool addValidation(std::string node, std::string sig);
  bool addValidation(DCValidationBlock& other);

  /** Calculates summary for this block based on provided transactions.
   *  @param a vector of transactions in this block
   *  @return the canonical form of the summary
  */
  //std::string CalculateSummary(const std::vector<DCTransaction>& txs);

/** Returns a JSON string representing this validation block.
 *  @return a JSON string representing this validation block.
*/
  std::string ToJSON() const;

/** Returns a CBOR byte vector representing this validation block.
 *  @return a CBOR byte vector representing this validation block.
*/
  std::vector<uint8_t> ToCBOR() const;

/** Returns the size of this verification block in bytes
 *  @return the size of this verification block in bytes
*/
  unsigned int GetByteSize() const;

/** Returns the number of validations in this block.
 *  @return the number of validations in this block.
*/
  unsigned int GetValidationCount() const;

/** Comparison Operators */
  friend bool operator==(const DCValidationBlock& a, const DCValidationBlock& b)
  {
      return a.hash_ == b.hash_;
  }

  friend bool operator!=(const DCValidationBlock& a, const DCValidationBlock& b)
  {
      return a.hash_ != b.hash_;
  }

/** Assignment Operators */
  DCValidationBlock* operator=(DCValidationBlock&& other)
  {
    if (this != &other) {
      this->summaryObj_ = other.summaryObj_;
      this->hash_ = other.hash_;
      this->jsonStr_ = other.jsonStr_;
      this->sigs_ = std::move(other.sigs_);
    }
    return this;
  }
  DCValidationBlock* operator=(DCValidationBlock& other)
  {
    if (this != &other) {
      this->summaryObj_ = other.summaryObj_;
      this->hash_ = other.hash_;
      this->jsonStr_ = other.jsonStr_;
      this->sigs_ = std::move(other.sigs_);
    }
    return this;
  }

/** Returns the hash of this validation block.
 *  @return the hash of this validation block.
*/
  const std::string& GetHash() const {
      return hash_;
  }

 private:
  std::string hash_ = "";
  std::string jsonStr_ = "";

/** Recalculates and returns the hash of this validation block.
 *  @return the hash of this validation block.
*/
  std::string ComputeHash() const;

};

} //end namespace Devcash

#endif /* SRC_PRIMITIVES_VALIDATION_H_ */
