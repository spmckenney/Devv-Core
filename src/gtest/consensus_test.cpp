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
      : t2_context_(1, 0, eAppMode::T2, "", "", "")
      , keys_(t2_context_)
      , proposal_chain_("prop-chain")
      , final_chain_("final-chain")
      , chain_state_()
      , utx_pool_ptr_() {
    for (int i = 0; i < 4; ++i) {
      keys_.addWalletKeyPair(kADDRs.at(i), kADDR_KEYs.at(i), "password");
    }
    keys_.setInnKeyPair(kINN_ADDR, kINN_KEY, "password");
    for (int i = 0; i < 3; ++i) {
      keys_.addNodeKeyPair(kNODE_ADDRs.at(i), kNODE_KEYs.at(i), "password");
    }
    utx_pool_ptr_ = std::make_unique<UnrecordedTransactionPool>(chain_state_, eAppMode::T2, 100);
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

  std::vector<byte> createTestProposal() {
    return CreateNextProposal(keys_,
                              proposal_chain_,
                              *utx_pool_ptr_,
                              t2_context_);
  }

  void handleValidationBlock(DevvMessageUniquePtr message) {
    LogDevvMessageSummary(*message, "UnrecordedTransactionPoolTest::handleValidationBlock");

    DevvMessageCallback cb = [this](DevvMessageUniquePtr p) { this->handleFinalBlock(std::move(p)); };

    HandleValidationBlock(std::move(message), t2_context_, final_chain_, *utx_pool_ptr_, cb);
  }

  void handleFinalBlock(DevvMessageUniquePtr message) {
    LogDevvMessageSummary(*message, "UnrecordedTransactionPoolTest::handleFinalBlock");
  }

  // Create a default context
  DevvContext t2_context_;
  KeyRing keys_;
  Blockchain proposal_chain_;
  Blockchain final_chain_;

  ChainState chain_state_;
  std::unique_ptr<UnrecordedTransactionPool> utx_pool_ptr_;
};

Tier2TransactionPtr CreateInnTransaction(KeyRing& keys, int64_t amount) {
  size_t addr_count = keys.CountWallets();
  Address inn_addr = keys.getInnAddr();

  std::string nonce_str = "Here is a very big test nonce for the UnrecordedTransactionPool class.";
  std::vector<byte> nonce_bin(nonce_str.begin(), nonce_str.end());

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

/**
 * Create a vector of txes with a single
 * @param keys
 * @return
 */
Tier2TransactionPtr CreateTestTransaction(const KeyRing& keys, int send_amount = -1, int recv_amount = 1) {
  std::vector<TransactionPtr> txs;

  Transfer receiver(keys.getWalletAddr(0), 0, recv_amount, 0);
  Transfer sender(keys.getWalletAddr(1), 0, send_amount, 0);

  std::vector<Transfer> xfers;
  xfers.push_back(sender);
  xfers.push_back(receiver);

  uint64_t nonce = 101101101;
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce, nonce_bin);
  auto t2x = std::make_unique<Tier2Transaction>(
      eOpType::Exchange, xfers,
      nonce_bin,
      keys.getWalletKey(1), keys);

  return t2x;
}

TEST_F(UnrecordedTransactionPoolTest, constructor_0) {
  CoinMap coin_map;
  DelayedItem delayed_item;
  AddToCoinMap(10, delayed_item, coin_map);

  ChainState chain_state;

  EXPECT_EQ(chain_state.getStateMap().size(), 0);

  //utx_pool_ptr_ = std::make_unique<UnrecordedTransactionPool>(chain_state, eAppMode::T2, 100);

  EXPECT_EQ(utx_pool_ptr_->getMode(), eAppMode::T2);
}

TEST_F(UnrecordedTransactionPoolTest, addTransaction_0) {
  std::unique_ptr<Tier2Transaction> inn_tx = CreateInnTransaction(keys_, 100);
  EXPECT_EQ(inn_tx->getOperation(), eOpType::Create);

  std::vector<TransactionPtr> inn_tx_vector;
  inn_tx_vector.push_back(std::move(inn_tx));

  utx_pool_ptr_->AddTransactions(inn_tx_vector, keys_);

  EXPECT_EQ(utx_pool_ptr_->numPendingTransactions(), 1);

  auto proposal = createTestProposal();
}

TEST_F(UnrecordedTransactionPoolTest, isNullProposal_0) {
  auto t2x = CreateTestTransaction(keys_);

  std::vector<TransactionPtr> inn_tx_vector;
  inn_tx_vector.push_back(std::move(t2x));

  utx_pool_ptr_->AddTransactions(inn_tx_vector, keys_);

  EXPECT_EQ(utx_pool_ptr_->numPendingTransactions(), 1);

  auto proposal = createTestProposal();

  EXPECT_EQ(ProposedBlock::isNullProposal(proposal), false);
}

TEST_F(UnrecordedTransactionPoolTest, validate_0) {
  auto t2x = CreateTestTransaction(keys_);

  std::vector<TransactionPtr> tx_vector;
  tx_vector.push_back(std::move(t2x));

  utx_pool_ptr_->AddTransactions(tx_vector, keys_);

  EXPECT_EQ(utx_pool_ptr_->numPendingTransactions(), 1);

  auto proposal = createTestProposal();

  EXPECT_EQ(ProposedBlock::isNullProposal(proposal), false);

  InputBuffer buffer(proposal);
  ProposedBlock to_validate(ProposedBlock::Create(buffer,
                                                  chain_state_,
                                                  keys_,
                                                  utx_pool_ptr_->get_transaction_creation_manager()));
  EXPECT_EQ(to_validate.getVersion(), 0);

  auto valid = to_validate.validate(keys_);

  EXPECT_TRUE(valid);
}


TEST_F(UnrecordedTransactionPoolTest, validate_1) {
  auto t2x = CreateTestTransaction(keys_, -2, 2);

  std::vector<TransactionPtr> inn_tx_vector;
  inn_tx_vector.push_back(std::move(t2x));

  utx_pool_ptr_->AddTransactions(inn_tx_vector, keys_);

  EXPECT_EQ(utx_pool_ptr_->numPendingTransactions(), 1);

  auto proposal = createTestProposal();

  EXPECT_EQ(ProposedBlock::isNullProposal(proposal), false);

  InputBuffer buffer(proposal);
  ProposedBlock to_validate(ProposedBlock::Create(buffer, chain_state_, keys_, utx_pool_ptr_->get_transaction_creation_manager()));
  auto valid = to_validate.validate(keys_);
  auto sign = to_validate.signBlock(keys_, t2_context_);
  EXPECT_TRUE(valid);
  EXPECT_TRUE(sign);
}

TEST_F(UnrecordedTransactionPoolTest, assymetric_0) {
  EXPECT_THROW(CreateTestTransaction(keys_, 2, 2), std::runtime_error);
}

TEST_F(UnrecordedTransactionPoolTest, finalize_0) {
  auto t2x = CreateTestTransaction(keys_, -2, 2);

  std::vector<TransactionPtr> inn_tx_vector;
  inn_tx_vector.push_back(std::move(t2x));

  utx_pool_ptr_->AddTransactions(inn_tx_vector, keys_);

  EXPECT_EQ(utx_pool_ptr_->numPendingTransactions(), 1);

  auto proposal = createTestProposal();

  EXPECT_EQ(ProposedBlock::isNullProposal(proposal), false);

  InputBuffer buffer(proposal);
  ProposedBlock to_validate(ProposedBlock::Create(buffer, chain_state_, keys_, utx_pool_ptr_->get_transaction_creation_manager()));
  auto valid = to_validate.validate(keys_);
  auto sign = to_validate.signBlock(keys_, t2_context_);
  EXPECT_TRUE(valid);
  EXPECT_TRUE(sign);

  EXPECT_TRUE(utx_pool_ptr_->CheckValidation(buffer, t2_context_));
/*
    //block can be finalized, so finalize
    LOG_DEBUG << "Ready to finalize block.";
    FinalPtr top_block = std::make_shared<FinalBlock>(utx_pool_ptr_->FinalizeLocalBlock());
    proposal_chain_.push_back(top_block);
*/
}

TEST_F(UnrecordedTransactionPoolTest, finalize_inn_tx) {
  std::unique_ptr<Tier2Transaction> inn_tx = CreateInnTransaction(keys_, 100);
  EXPECT_EQ(inn_tx->getOperation(), eOpType::Create);

  std::vector<TransactionPtr> inn_tx_vector;
  inn_tx_vector.push_back(std::move(inn_tx));

  utx_pool_ptr_->AddTransactions(inn_tx_vector, keys_);

  EXPECT_EQ(utx_pool_ptr_->numPendingTransactions(), 1);

  auto proposal = createTestProposal();

  EXPECT_EQ(ProposedBlock::isNullProposal(proposal), false);

  InputBuffer buffer(proposal);
  ProposedBlock to_validate(ProposedBlock::Create(buffer, chain_state_, keys_, utx_pool_ptr_->get_transaction_creation_manager()));

  EXPECT_EQ(to_validate.getVersion(), 0);

  std::cout << "VERSION: " << int(to_validate.getVersion()) << " -- " << std::endl;

  auto valid = to_validate.validate(keys_);
  auto sign = to_validate.signBlock(keys_, t2_context_);
  EXPECT_TRUE(valid);
  EXPECT_TRUE(sign);

  size_t block_height = final_chain_.size();

  EXPECT_EQ(final_chain_.size(), 0);
  EXPECT_EQ(proposal_chain_.size(), 0);

  // Create message
  auto propose_msg =
      std::make_unique<DevvMessage>(t2_context_.get_shard_uri(),
                                    PROPOSAL_BLOCK,
                                    proposal,
                                    ((block_height + 1) + (t2_context_.get_current_node() + 1) * 1000000));

  DevvMessageCallback cb = [this](DevvMessageUniquePtr p) { this->handleValidationBlock(std::move(p)); };

  HandleProposalBlock(std::move(propose_msg),
                      t2_context_,
                      keys_,
                      proposal_chain_,
                      utx_pool_ptr_->get_transaction_creation_manager(), cb);
}

TEST_F(UnrecordedTransactionPoolTest, finalize_tx_1) {
auto t2x = CreateTestTransaction(keys_, -2, 2);
std::unique_ptr<Tier2Transaction> inn_tx = CreateInnTransaction(keys_, 100);
EXPECT_EQ(inn_tx->getOperation(), eOpType::Create);

std::vector<TransactionPtr> inn_tx_vector;
inn_tx_vector.push_back(std::move(inn_tx));

utx_pool_ptr_->AddTransactions(inn_tx_vector, keys_);

EXPECT_EQ(utx_pool_ptr_->numPendingTransactions(), 1);

auto proposal = createTestProposal();

EXPECT_EQ(ProposedBlock::isNullProposal(proposal), false);

InputBuffer buffer(proposal);
ProposedBlock to_validate(ProposedBlock::Create(buffer, chain_state_, keys_, utx_pool_ptr_->get_transaction_creation_manager()));
auto valid = to_validate.validate(keys_);
auto sign = to_validate.signBlock(keys_, t2_context_);
EXPECT_TRUE(valid);
EXPECT_TRUE(sign);

size_t block_height = final_chain_.size();

EXPECT_EQ(final_chain_.size(), 0);
EXPECT_EQ(proposal_chain_.size(), 0);

// Create message
auto propose_msg =
    std::make_unique<DevvMessage>(t2_context_.get_shard_uri(),
                                  PROPOSAL_BLOCK,
                                  proposal,
                                  ((block_height + 1) + (t2_context_.get_current_node() + 1) * 1000000));

DevvMessageCallback cb = [this](DevvMessageUniquePtr p) { this->handleValidationBlock(std::move(p)); };

HandleProposalBlock(std::move(propose_msg), t2_context_, keys_, proposal_chain_, utx_pool_ptr_->get_transaction_creation_manager(), cb);
}

TEST_F(UnrecordedTransactionPoolTest, proposal_stream_0) {
  auto t2x = CreateTestTransaction(keys_, -2, 2);

  std::vector<TransactionPtr> inn_tx_vector;
  inn_tx_vector.push_back(std::move(t2x));

  utx_pool_ptr_->AddTransactions(inn_tx_vector, keys_);

  EXPECT_EQ(utx_pool_ptr_->numPendingTransactions(), 1);

  auto proposal = createTestProposal();

  InputBuffer buffer(proposal);
  auto proposal2 = ProposedBlock::Create(buffer, chain_state_, keys_, utx_pool_ptr_->get_transaction_creation_manager());

  EXPECT_EQ(proposal, proposal2.getCanonical());
}

} // namespace
} // namespace Devv
