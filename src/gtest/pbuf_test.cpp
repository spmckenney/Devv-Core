/*
 * pbuf_test.cpp
 *
 *  Created on: 8/20/18
 *      Author: mckenney
 */
#include <cstdint>

#include "gtest/gtest.h"

#include "primitives/Summary.h"
#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"
#include "primitives/Transfer.h"
#include "primitives/json_interface.h"
#include "primitives/block_tools.h"

#include "pbuf/devv_pbuf.h"

namespace {

#define TEST_DESCRIPTION(desc) RecordProperty("unit tests for pbuf data types and converters", desc)

/**
 *
 * InputBufferTest
 *
 */
class PbufTransactionTest : public ::testing::Test {
 protected:
  PbufTransactionTest() = default;

  ~PbufTransactionTest() = default;

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }
};

TEST_F(PbufTransactionTest, defaultConstructor) {
  Devv::proto::Transaction pb_tx;

  EXPECT_EQ(pb_tx.nonce_size(), 0);
}

} // unnamed namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
