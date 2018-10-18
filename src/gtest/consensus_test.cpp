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
#include "consensus/UnrecordedTransactionPool.h"
#include "consensus/tier2_message_handlers.h"

namespace Devv {
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
                                   "", "", ""), keys_(t1_context_0_) {
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

/**
 *
 * UnrecordedTransactionPoolTest
 *
 */
class UnrecordedTransactionPoolTest : public ::testing::Test {
 protected:
  UnrecordedTransactionPoolTest()
      : t1_context_(0, 0, eAppMode::T1, "", "", "")
      , keys_(t1_context_)
      , chain_state_()
      , pool_ptr_() {
    for (int i = 0; i < 4; ++i) {
      keys_.addWalletKeyPair(kADDRs.at(i), kADDR_KEYs.at(i), "password");
    }
    keys_.setInnKeyPair(kINN_ADDR, kINN_KEY, "password");
    for (int i = 0; i < 3; ++i) {
      keys_.addNodeKeyPair(kNODE_ADDRs.at(i), kNODE_KEYs.at(i), "password");
    }
  }

  ~UnrecordedTransactionPoolTest() override = default;

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Create a default context
  DevvContext t1_context_;
  KeyRing keys_;

  ChainState chain_state_;
  std::unique_ptr<UnrecordedTransactionPool> pool_ptr_;
};

Tier2TransactionPtr CreateInnTransaction(KeyRing& keys, int64_t amount) {
  size_t addr_count = keys.CountWallets();
  Address inn_addr = keys.getInnAddr();

  uint64_t nonce_num = GetMillisecondsSinceEpoch() + 1000011;
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce_num, nonce_bin);

  std::vector<Transfer> xfers;
  Transfer inn_transfer(inn_addr, 0, -1 * addr_count * amount, 0);
  xfers.push_back(inn_transfer);
  for (size_t i = 0; i < addr_count; ++i) {
    Transfer transfer(keys.getWalletAddr(i), 0, amount, 0);
    xfers.push_back(transfer);
  }

  Tier2TransactionPtr inn_tx = std::make_unique<Tier2Transaction>(eOpType::Create, xfers, nonce_bin,
                                                                  keys.getKey(inn_addr), keys);

  return inn_tx;
}


TEST_F(UnrecordedTransactionPoolTest, constructor_0) {
  CoinMap coin_map;
  DelayedItem delayed_item;
  AddToCoinMap(10, delayed_item, coin_map);

  ChainState chain_state;

  EXPECT_EQ(chain_state.getStateMap().size(), 0);

  pool_ptr_ = std::make_unique<UnrecordedTransactionPool>(chain_state, eAppMode::T2, 100);

  EXPECT_EQ(pool_ptr_->getMode(), eAppMode::T2);
}

TEST_F(UnrecordedTransactionPoolTest, addTransaction_0) {
  CoinMap coin_map;
  DelayedItem delayed_item;
  AddToCoinMap(10, delayed_item, coin_map);

  ChainState chain_state;

  EXPECT_EQ(chain_state.getStateMap().size(), 0);

  pool_ptr_ = std::make_unique<UnrecordedTransactionPool>(chain_state, eAppMode::T2, 100);

  std::unique_ptr<Tier2Transaction> inn_tx = CreateInnTransaction(keys_, 100);
  EXPECT_EQ(inn_tx->getOperation(), eOpType::Create);

  std::vector<TransactionPtr> inn_tx_vector;
  inn_tx_vector.push_back(std::move(inn_tx));

  pool_ptr_->AddTransactions(inn_tx_vector, keys_);

  EXPECT_EQ(pool_ptr_->numPendingTransactions(), 1);

  Blockchain blockchain("test-chain");
  std::vector<byte> proposal = CreateNextProposal(keys_,
                                                  blockchain,
                                                  *pool_ptr_,
                                                  t1_context_);
}

} // namespace
} // namespace Devv
