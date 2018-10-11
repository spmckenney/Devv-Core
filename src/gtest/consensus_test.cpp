/*
 * consensus_test.cpp tests consensus logic of Devv validators.
 *
 * @copywrite  2018 Devvio Inc
 */

#include "gtest/gtest.h"

#include "primitives/Summary.h"
#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"
#include "primitives/Transfer.h"
#include "primitives/json_interface.h"

#include "consensus/chainstate.h"

namespace {

#define TEST_DESCRIPTION(desc) RecordProperty("consensus algorithm unit tests", desc)

/**
 *
 * ChainStateTest
 *
 */
class ChainStateTest : public ::testing::Test {
 protected:
  ChainStateTest() : t1_context_0_(0, 0, Devv::eAppMode::T1,
                                   "", "", "")
  , keys_(t1_context_0_) {
    for (int i = 0; i < 4; ++i) {
      keys_.addWalletKeyPair(kADDRs.at(i), kADDR_KEYs.at(i), "password");
    }
    keys_.setInnKeyPair(kINN_ADDR, kINN_KEY, "password");
    for (int i = 0; i < 3; ++i) {
      keys_.addNodeKeyPair(kNODE_ADDRs.at(i), kNODE_KEYs.at(i), "password");
    }
  }

  ~ChainStateTest() override = default;

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Create a default context
  Devv::DevvContext t1_context_0_;
  Devv::KeyRing keys_;
};

TEST_F(ChainStateTest, constructor_0) {
  CoinMap coin_map;
  DelayedItem delayed_item;
  AddToCoinMap(10, delayed_item, coin_map);

  ChainState chain_state;

  EXPECT_EQ(chain_state.getStateMap().size(), 0);
  //CoinMap inner(coin.getCoin(), coin.getAmount());
  //std::pair<Address, CoinMap> outer(coin.getAddress(),inner);
  //auto result = state_map_.insert(outer);
}

TEST_F(ChainStateTest, addCoin_0) {

  SmartCoin coin(keys_.getWalletAddr(0), 10, 10);

  ChainState chain_state;
  chain_state.addCoin(coin);

  EXPECT_EQ(chain_state.getStateMap().size(), 1);
}

} // namespace
