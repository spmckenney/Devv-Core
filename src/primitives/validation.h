/*
 * validation.h
 *
 *  Created on: Jan 3, 2018
 *  Author: Nick Williams
 *
 *  **Validation Structure**
 *  A map of miner addresses to signatures.
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
#include <unordered_map>
#include <vector>
#include <string>

typedef std::unordered_map<std::string, std::string> vmap;

class DCValidationBlock {
public:
  vmap sigs;

    bool IsNull() const { return (sigs.size() < 1); }

    DCValidationBlock();
    DCValidationBlock(const char* cbor);
    DCValidationBlock(std::string jsonMsg);
    DCValidationBlock(const DCValidationBlock& other);
  DCValidationBlock(std::string node, std::string sig);
  DCValidationBlock(std::unordered_map<std::string, std::string> sigs);

  bool addValidation(std::string node, std::string sig);
  std::string ToJSON() const;
  std::vector<uint8_t> ToCBOR() const;

    unsigned int GetByteSize() const;
    unsigned int GetValidationCount() const;
    friend bool operator==(const DCValidationBlock& a, const DCValidationBlock& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const DCValidationBlock& a, const DCValidationBlock& b)
    {
        return a.hash != b.hash;
    }
    DCValidationBlock* operator=(DCValidationBlock& other)
  {
      if (this != &other) {
      this->hash = other.hash;
      this->jsonStr = other.jsonStr;
      this->sigs = std::move(other.sigs);
      }
      return this;
    }
private:
    std::string hash = "";
    std::string jsonStr = "";
    std::string ComputeHash() const;

public:
    const std::string& GetHash() const {
        return hash;
    }
};

#endif /* SRC_PRIMITIVES_VALIDATION_H_ */
