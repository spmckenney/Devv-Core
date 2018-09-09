/*
 * devcash_constants.h
 *
 *  Created on: 6/22/18
 *      Author: mckenney
 */
#pragma once

namespace Devcash {

static const unsigned int kDEFAULT_WORKERS = 8;

// millis of sleep between main shut down checks
static const int kMAIN_WAIT_INTERVAL = 100;

static const size_t kWALLET_ADDR_SIZE = 33;
static const size_t kNODE_ADDR_SIZE = 49;

static const size_t kWALLET_ADDR_BUF_SIZE = kWALLET_ADDR_SIZE + 1;
static const size_t kNODE_ADDR_BUF_SIZE = kNODE_ADDR_SIZE + 1;

static const size_t kFILE_KEY_SIZE = 379;
static const size_t kFILE_NODEKEY_SIZE = 448;

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

} // namespace Devcash
