/*
 * block.h
 *
 *  Created on: Dec 11, 2017
 *      Author: Nick Williams
 *
 *      **Block Structure**
 *
 *      -Headers-
 *		nVersion
 *		previous blockhash
 *		hashMerkleRoot
 *		blocksize (nBits)
 *		timestamp (nTime)
 *		transaction size (bytes)
 *		validation size (bytes)
 *
 *		-Transactions-
 *		-Validations-
 */

#ifndef DEVCASH_PRIMITIVES_BLOCK_H
#define DEVCASH_PRIMITIVES_BLOCK_H

#include "transaction.h"
#include "validation.h"
//#include "../ossladapter.h";

/*struct DCValidation
{
    std::string miner;
    std::string valid;
};*/

class DCBlockHeader
{
public:
    // header
    short nVersion = 0;
    std::string hashPrevBlock;
    std::string hashMerkleRoot;
    uint32_t nBytes;
    uint64_t nTime;
    uint32_t txSize;
    uint32_t vSize;

    DCBlockHeader()
    {
        SetNull();
    }

    DCBlockHeader(std::string hashPrevBlock, std::string hashMerkleRoot, uint32_t nBytes, uint64_t nTime, uint32_t txSize, uint32_t vSize)
    {
    	this->hashPrevBlock=hashPrevBlock;
    	this->hashMerkleRoot=hashMerkleRoot;
    	this->nBytes=nBytes;
    	this->nTime=nTime;
    	this->txSize=txSize;
    	this->vSize=vSize;
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock = "";
        hashMerkleRoot = "";
        nTime = 0;
        nBytes = 0;
        txSize = 0;
        vSize = 0;
    }

    bool IsNull() const
    {
        return (nBytes == 0);
    }

    std::string GetHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    std::string ToJSON() const;
    std::string ToCBOR() const;
};


class DCBlock : public DCBlockHeader
{
public:
    std::vector<DCTransaction> &vtx;
    DCValidationBlock &vals;
    mutable bool fChecked;

    DCBlock(std::vector<DCTransaction> &txs, DCValidationBlock &validations);

    bool validate(EC_KEY* eckey);
    bool signBlock(EC_KEY* eckey);
    bool finalize();

    void SetNull()
    {
        DCBlockHeader::SetNull();
        fChecked = false;
    }

    DCBlockHeader GetBlockHeader() const
    {
        DCBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBytes          = nBytes;
        return block;
    }

    std::string ToJSON() const;
    std::vector<uint8_t> ToCBOR() const;
    std::string ToCBOR_str();
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct DCBlockLocator
{
    std::vector<std::string> vHave;

    DCBlockLocator() {}

    explicit DCBlockLocator(const std::vector<std::string>& vHaveIn) : vHave(vHaveIn) {}

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // DEVCASH_PRIMITIVES_BLOCK_H

