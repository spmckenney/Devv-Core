/*
 * devv_constants.h
 *
 *  Created on: 6/22/18
 *      Author: mckenney
 */
#pragma once

namespace Devv {

static const unsigned int kDEFAULT_WORKERS = 8;

// millis of sleep between main shut down checks
static const int kMAIN_WAIT_INTERVAL = 100;

static const size_t kWALLET_ADDR_SIZE = 33;
static const size_t kNODE_ADDR_SIZE = 49;

static const size_t kWALLET_ADDR_BUF_SIZE = kWALLET_ADDR_SIZE + 1;
static const size_t kNODE_ADDR_BUF_SIZE = kNODE_ADDR_SIZE + 1;

static const size_t kFILE_KEY_SIZE = 379;
static const size_t kFILE_NODEKEY_SIZE = 448;

//Transaction constants

/**
 * Types of operations performed by transactions
 */
enum eOpType : byte { Create = 0, Modify = 1, Exchange = 2, Delete = 3, NumOperations = 4 };

static const std::string kVERSION_TAG = "v";
static const std::string kPREV_HASH_TAG = "prev";
static const std::string kMERKLE_TAG = "merkle";
static const std::string kBYTES_TAG = "bytes";
static const std::string kTIME_TAG = "time";
static const std::string kTX_SIZE_TAG = "txlen";
static const std::string kVAL_COUNT_TAG = "vcount";
static const std::string kTXS_TAG = "txs";
static const std::string kSUM_TAG = "sum";
static const std::string kVAL_TAG = "vals";
static const std::string kXFER_SIZE_TAG = "xfer_size";
static const std::string kSUMMARY_TAG = "summary";
static const std::string kSUM_SIZE_TAG = "sum_size";
static const std::string kOPER_TAG = "oper";
static const std::string kXFER_TAG = "xfer";
static const std::string kNONCE_TAG = "nonce";
static const std::string kNONCE_SIZE_TAG = "nonce_size";
static const std::string kSIG_TAG = "sig";
static const std::string kVALIDATOR_DEX_TAG = "val_dex";

static const size_t kTX_MIN_SIZE = (89 + 2);
static const size_t kENVELOPE_SIZE = 17;
static const size_t kOPERATION_OFFSET = 16;
static const size_t kTRANSFER_OFFSET = 17;
static const size_t kMIN_NONCE_SIZE = 8;
static const size_t kUINT64_SIZE = 8;
static const size_t kTRANSFER_NONADDR_DATA_SIZE = kUINT64_SIZE*3;

} // namespace Devv
