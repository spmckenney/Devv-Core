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

} // namespace Devcash
