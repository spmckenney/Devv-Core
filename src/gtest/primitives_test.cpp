//
// Created by mckenney on 6/11/18.
//

#include "gtest/gtest.h"

#include "primitives/Summary.h"
#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"
#include "primitives/Transfer.h"
#include "primitives/json_interface.h"
#include "primitives/block_tools.h"

namespace {

#define TEST_DESCRIPTION(desc) RecordProperty("primitive data type unit tests", desc)

TEST(converters, Int32ToBin_0) {
  std::vector<byte> buf;
  int32_t i = 100;
  Int32ToBin(i, buf);
  int32_t i2 = BinToInt32(buf, 0);
  EXPECT_EQ(i, i2);
}

TEST(converters, Int32ToBin_1) {
  std::vector<byte> buf;
  int32_t i = INT32_MAX - 1;
  Int32ToBin(i, buf);
  int32_t i2 = BinToInt32(buf, 0);
  EXPECT_EQ(i, i2);
}

TEST(converters, UInt32ToBin_0) {
  std::vector<byte> buf;
  uint32_t i = 10101;
  Uint32ToBin(i, buf);
  uint32_t i2 = BinToUint32(buf, 0);
  EXPECT_EQ(i, i2);
}

TEST(converters, UInt32ToBin_1) {
  std::vector<byte> buf;
  uint32_t i = UINT32_MAX - 1;
  Uint32ToBin(i, buf);
  uint32_t i2 = BinToUint32(buf, 0);
  EXPECT_EQ(i, i2);
}

TEST(converters, Int64ToBin_0) {
  std::vector<byte> buf;
  int64_t i = 100;
  Int64ToBin(i, buf);
  int64_t i2 = BinToInt64(buf, 0);
  EXPECT_EQ(i, i2);
}

TEST(converters, Int64ToBin_1) {
  std::vector<byte> buf;
  int64_t i = INT64_MAX - 1;
  Int64ToBin(i, buf);
  int64_t i2 = BinToInt64(buf, 0);
  EXPECT_EQ(i, i2);
}

TEST(converters, UInt64ToBin_0) {
  std::vector<byte> buf;
  uint64_t i = 10101;
  Uint64ToBin(i, buf);
  uint64_t i2 = BinToUint64(buf, 0);
  EXPECT_EQ(i, i2);
}

TEST(converters, UInt64ToBin_1) {
  std::vector<byte> buf;
  uint64_t i = UINT64_MAX - 1;
  Uint64ToBin(i, buf);
  uint64_t i2 = BinToUint64(buf, 0);
  EXPECT_EQ(i, i2);
}

/**
 *
 * InputBufferTest
 *
 */
 class InputBufferTest : public ::testing::Test {
 protected:
  InputBufferTest() : test_buffer_0_()
  {
    test_buffer_0_.push_back(10);
  }

  ~InputBufferTest() override = default;

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  std::vector<byte> test_buffer_0_;
};

TEST_F(InputBufferTest, checkAt) {
  InputBuffer ibuffer(test_buffer_0_);
  byte b = 10;
  EXPECT_EQ(b, ibuffer.at(0));
}

TEST_F(InputBufferTest, test_copy_0) {
  std::vector<byte> check_vec;
  InputBuffer ibuffer(test_buffer_0_);
  ibuffer.copy(std::back_inserter(check_vec), 1);
  EXPECT_EQ(test_buffer_0_, check_vec);
}

TEST_F(InputBufferTest, test_copy_1) {
  std::vector<byte> b1;
  b1.push_back(34);
  b1.push_back(35);

  std::array<byte, 2> ar1, ar2;
  ar1[0] = b1.at(0);
  ar1[1] = b1.at(1);
  InputBuffer ibuffer(b1);
  ibuffer.copy(ar2);
  EXPECT_EQ(ar1, ar2);
}

TEST_F(InputBufferTest, test_copy_2) {
  std::vector<byte> b1;
  b1.push_back(34);
  b1.push_back(35);
  b1.push_back(36);
  b1.push_back(37);
  InputBuffer ibuffer(b1);

  std::array<byte, 2> ar1, ar2;

  ar1[0] = b1.at(0);
  ar1[1] = b1.at(1);
  ibuffer.copy(ar2);
  EXPECT_EQ(ar1, ar2);

  ar1[0] = b1.at(2);
  ar1[1] = b1.at(3);
  ibuffer.copy(ar2);
  EXPECT_EQ(ar1, ar2);
}

TEST_F(InputBufferTest, test_getOffset) {
  byte int0 = 1;
  byte int1 = 8;
  std::vector<byte> b1;
  b1.push_back(int0);
  b1.push_back(int1);

  InputBuffer ibuffer(b1);

  size_t check = 0;
  EXPECT_EQ(check, ibuffer.getOffset());
  ibuffer.getNextByte();
  check++;
  EXPECT_EQ(check, ibuffer.getOffset());
}

TEST_F(InputBufferTest, test_increment) {
  byte int0 = 1;
  byte int1 = 8;
  std::vector<byte> b1;
  b1.push_back(int0);
  b1.push_back(int1);

  InputBuffer ibuffer(b1);

  byte check;
  ibuffer.increment(1);
  check = ibuffer.getNextByte();
  EXPECT_EQ(int1, check);
}

TEST_F(InputBufferTest, test_byte) {
  byte int0 = 1;
  byte int1 = 8;
  std::vector<byte> b1;
  b1.push_back(int0);
  b1.push_back(int1);

  InputBuffer ibuffer(b1);

  byte check;
  check = ibuffer.getNextByte();
  EXPECT_EQ(int0, check);

  check = ibuffer.getNextByte();
  EXPECT_EQ(int1, check);
}

TEST_F(InputBufferTest, test_uint32) {
  uint32_t int0 = 123456;
  uint32_t int1 = 987654;
  std::vector<byte> b1;
  Uint32ToBin(int0, b1);
  Uint32ToBin(int1, b1);

  InputBuffer ibuffer(b1);

  uint32_t check;
  check = ibuffer.getNextUint32();
  EXPECT_EQ(int0, check);

  check = ibuffer.getNextUint32();
  EXPECT_EQ(int1, check);
}

TEST_F(InputBufferTest, test_uint64) {
  uint64_t int0 = 123456;
  uint64_t int1 = 987654;
  std::vector<byte> b1;
  Uint64ToBin(int0, b1);
  Uint64ToBin(int1, b1);

  InputBuffer ibuffer(b1);

  uint64_t check;
  check = ibuffer.getNextUint64();
  EXPECT_EQ(int0, check);

  check = ibuffer.getNextUint64();
  EXPECT_EQ(int1, check);
}

/**
 *
 * SummaryTest
 *
 */
 class SummaryTest : public ::testing::Test {
 protected:
  SummaryTest() : t1_context_0_(0, 0, Devcash::eAppMode::T1, "", "", "")
      , addr_0_(Devcash::Hex2Bin(t1_context_0_.kADDRs[0]))
  {
  }

  ~SummaryTest()  = default;

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Create a default context
  Devcash::DevcashContext t1_context_0_;

  // create a default address
  Devcash::Address addr_0_;
};

TEST_F(SummaryTest, checkMinSize) {
  size_t size = 4;
  EXPECT_EQ(size, Summary::MinSize());
}

TEST_F(SummaryTest, defaultConstructor) {
  Summary s = Summary::Create();

  size_t zero = 0;
  std::vector<Devcash::byte> canon(Summary::MinSize(), '\0');
  EXPECT_EQ(s.getCanonical(), canon);
  EXPECT_EQ(s.getByteSize(), Summary::MinSize());
  EXPECT_EQ(s.getTransferCount(), zero);
  EXPECT_EQ(s.getTransfers().size(), zero);
  Devcash::Address addr;
  EXPECT_EQ(s.getCoinsByAddr(addr, zero).size(), zero);
  EXPECT_FALSE(s.isSane());
}


TEST_F(SummaryTest, addItem_0) {
  TEST_DESCRIPTION("Tests addItem() with no delay and checks size");
  Summary s = Summary::Create();

  /// @todo (mckenney) maybe this should be calculated instead of being
  /// a magic number
  size_t bytes_per_item = 70;
  s.addItem(addr_0_, 10, 10);
  EXPECT_EQ(s.getByteSize(), bytes_per_item);
}

TEST_F(SummaryTest, addItem_1) {
  TEST_DESCRIPTION("Tests addItem() with delay and checks size");
  Summary s = Summary::Create();

  /// @todo (mckenney) maybe this should be calculated instead of being
  /// a magic number
  size_t bytes_per_item = 78;
  s.addItem(addr_0_, 1, 2, 3);
  EXPECT_EQ(s.getByteSize(), bytes_per_item);
}

TEST_F(SummaryTest, addItem_2) {
  TEST_DESCRIPTION("Tests addItem(DelayedItem(delay=10)) and checks size");
  Summary s = Summary::Create();

  /// @todo (mckenney) maybe this should be calculated instead of being
  /// a magic number
  uint64_t delay = 10;
  uint64_t delta = 0;
  DelayedItem item(delay, delta);

  size_t bytes_per_item = 78;
  s.addItem(addr_0_, 10, item);
  EXPECT_EQ(s.getByteSize(), bytes_per_item);
}

TEST_F(SummaryTest, addItem_3) {
  TEST_DESCRIPTION("Tests addItem(DelayedItem(delay=0)) and checks size");
  Summary s = Summary::Create();

  /// @todo (mckenney) maybe this should be calculated instead of being
  /// a magic number
  uint64_t delay = 0;
  uint64_t delta = 10;
  DelayedItem item(delay, delta);

  size_t bytes_per_item = 70;
  s.addItem(addr_0_, 10, item);
  EXPECT_EQ(s.getByteSize(), bytes_per_item);
}

TEST_F(SummaryTest, addItem_4) {
  TEST_DESCRIPTION("Tests adding two items with same address and checks size");
  Summary s = Summary::Create();

  /// @todo (mckenney) maybe this should be calculated instead of being
  /// a magic number
  uint64_t delay = 0;
  uint64_t delta = 10;
  DelayedItem item(delay, delta);

  uint64_t coin0 = 10;
  uint64_t coin1 = coin0;

  size_t size0 = 70;
  size_t size1 = 70;
  size_t transfer_count = 1;

  s.addItem(addr_0_, coin0, item);
  EXPECT_EQ(s.getByteSize(), size0);
  EXPECT_EQ(s.getTransferCount(), transfer_count);
  s.addItem(addr_0_, coin1, item);
  EXPECT_EQ(s.getByteSize(), size1);
  EXPECT_EQ(s.getTransferCount(), transfer_count);
}

TEST_F(SummaryTest, addItem_5) {
  TEST_DESCRIPTION("Tests adding two items with same coin and checks size");
  Summary s = Summary::Create();

  /// @todo (mckenney) maybe this should be calculated instead of being
  /// a magic number
  uint64_t coin0 = 10;
  uint64_t coin1 = coin0;

  size_t size0 = 70;
  size_t size1 = 70;
  size_t transfer_count = 1;

  bool add_item = s.addItem(addr_0_, coin0, 10);
  EXPECT_EQ(s.getByteSize(), size0);
  EXPECT_EQ(s.getTransferCount(), transfer_count);
  EXPECT_TRUE(add_item);

  add_item = s.addItem(addr_0_, coin1, 11);
  EXPECT_EQ(s.getByteSize(), size1);
  EXPECT_EQ(s.getTransferCount(), transfer_count);
  /// @todo (mckenney) should addItem return true/false when a coin is updated?
  EXPECT_TRUE(add_item);
}

TEST_F(SummaryTest, addItem_6) {
  TEST_DESCRIPTION("Tests adding two items with same address and checks size");
  Summary s = Summary::Create();

  /// @todo (mckenney) maybe this should be calculated instead of being
  /// a magic number
  uint64_t coin0 = 10;
  uint64_t coin1 = 11;

  size_t size0 = 70;
  size_t size1 = 86;
  size_t transfer_count = 1;

  s.addItem(addr_0_, coin0, 10);
  EXPECT_EQ(s.getByteSize(), size0);
  EXPECT_EQ(s.getTransferCount(), transfer_count);
  s.addItem(addr_0_, coin1, 11);
  EXPECT_EQ(s.getByteSize(), size1);
  EXPECT_EQ(s.getTransferCount(), transfer_count + 1);
}


/**
 *
 * TransferTest
 *
 */
class TransferTest : public ::testing::Test {
 protected:
  TransferTest() : t1_context_0_(0, 0, Devcash::eAppMode::T1, "", "", "")
      , addr_0_(Devcash::Hex2Bin(t1_context_0_.kADDRs[0]))
  {
  }

  // You can do clean-up work that doesn't throw exceptions here.
  ~TransferTest()  = default;

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case for Foo.

  // Create a default context
  Devcash::DevcashContext t1_context_0_;

  // create a default address
  Devcash::Address addr_0_;
};

TEST_F(TransferTest, getJSONIdentity) {
  Devcash::Transfer test_transfer(addr_0_, 0, 1, 0);
  InputBuffer buffer(test_transfer.getCanonical());
  Devcash::Transfer identity(buffer);

  EXPECT_EQ(test_transfer.getJSON(), identity.getJSON());
}

TEST_F(TransferTest, getCanonicalIdentity) {
  Devcash::Transfer test_transfer(addr_0_, 0, 1, 0);
  InputBuffer buffer(test_transfer.getCanonical());
  Devcash::Transfer identity(buffer);

  EXPECT_EQ(test_transfer.getCanonical(), identity.getCanonical());
}

TEST_F(TransferTest, getAddress_0) {
  std::vector<Devcash::byte> tmp(33, 'A');
  Address a(tmp);
  Devcash::Transfer test0(a, 0, 1, 0);
  EXPECT_EQ(test0.getAddress(), a);
}

TEST_F(TransferTest, getAddress_1) {
  Devcash::Transfer test0(addr_0_, 0, 1, 0);
  std::vector<Devcash::byte> tmp(Devcash::Hex2Bin(t1_context_0_.kADDRs[0]));
  Address a(tmp);
  EXPECT_EQ(test0.getAddress(), a);
}

// Exception - wrong size input
/*
 * TEST_F(TransferTest, getAddress_2) {
  Devcash::Transfer test0(addr_0_, 0, 1, 0);
  std::vector<Devcash::byte> tmp(Devcash::Hex2Bin(""));
  std::vector<byte> bin_addr(addr_0_.getCanonical());
  std::copy_n(tmp.begin(), Devcash::kWALLET_ADDR_SIZE, bin_addr.begin());
  EXPECT_THROW(Address a(tmp), std::runtime_error);
}
*/

TEST(ossl, SignBinary_0) {
  // Create a default context
  Devcash::DevcashContext context(0, 0, Devcash::eAppMode::T1, "", "", "");

  // keys
  Devcash::KeyRing keys(context);

  for (int i = 0; i < 4; ++i) {
    keys.addWalletKeyPair(context.kADDRs.at(i), context.kADDR_KEYs.at(i), "password");
  }

  keys.setInnKeyPair(context.kINN_ADDR, context.kINN_KEY, "password");

  for (int i = 0; i < 3; ++i) {
    keys.addNodeKeyPair(context.kNODE_ADDRs.at(i), context.kNODE_KEYs.at(i), "password");
  }

  Summary summary = Summary::Create();
  Address a = keys.getInnAddr();
  Address w = keys.getWalletAddr(0);
  summary.addItem(a, (uint64_t) 12, (int64_t) -47);
  summary.addItem(w, (uint64_t) 12, (int64_t) 47);
  std::vector<byte> msg = summary.getCanonical();
  auto k = keys.getKey(a);
  auto h = DevcashHash(msg);
  auto s1 = SignBinary(k, h);
  auto s2 = SignBinary(k, h);
  EXPECT_EQ(s1, s2);
}

/**
 *
 * Tier1TransactionTest
 *
 */
class Tier1TransactionTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  Tier1TransactionTest()
      : t1_context_0_(0, 0, Devcash::eAppMode::T1, "", "", "")
      , keys_(t1_context_0_)
  {
    for (int i = 0; i < 4; ++i) {
      keys_.addWalletKeyPair(t1_context_0_.kADDRs.at(i), t1_context_0_.kADDR_KEYs.at(i), "password");
    }
    keys_.setInnKeyPair(t1_context_0_.kINN_ADDR, t1_context_0_.kINN_KEY, "password");
    for (int i = 0; i < 3; ++i) {
      keys_.addNodeKeyPair(t1_context_0_.kNODE_ADDRs.at(i), t1_context_0_.kNODE_KEYs.at(i), "password");
    }
  }

  // You can do clean-up work that doesn't throw exceptions here.
  ~Tier1TransactionTest()  = default;

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case for Foo.

  // Create a default context
  Devcash::DevcashContext t1_context_0_;

  // keys
  Devcash::KeyRing keys_;
};

TEST_F(Tier1TransactionTest, defaultConstructor) {
  size_t zero = 0;
  Devcash::Tier1Transaction transaction = Tier1Transaction::Create();

  // @todo (mckenney) should be Devcash::Tier1Transaction::MinSize()?
  EXPECT_EQ(transaction.getByteSize(), zero);
}

Tier1TransactionPtr CreateTestT1_0(const KeyRing& keys) {
  Hash prev_hash;
  std::vector<TransactionPtr> txs;
  Summary sum_test = Summary::Create();
  Validation val_test = Validation::Create();
  ChainState state;

  ProposedBlock proposal_test(prev_hash, txs, sum_test, val_test, state);
  FinalBlock final_block(proposal_test);

  return(CreateTier1Transaction(final_block, keys));
}

Tier1Transaction CreateTestT1_1(const KeyRing& keys) {
  Summary summary = Summary::Create();
  Address a = keys.getInnAddr();
  Address w = keys.getWalletAddr(0);
  summary.addItem(a, (uint64_t) 12, (int64_t) -47);
  summary.addItem(w, (uint64_t) 12, (int64_t) 47);
  std::vector<byte> msg = summary.getCanonical();
  auto s = SignBinary(keys.getKey(a), DevcashHash(msg));
  Tier1Transaction tx1(summary, s, a, keys);
  InputBuffer ib(tx1.getCanonical());
  Tier1Transaction c = Tier1Transaction::Create(ib, keys, true);
  LOG_INFO << "Original addr: " + ToHex(a);
  LOG_INFO << "Final addr: " + ToHex(c.getNodeAddress());
  return c;
}

Tier1TransactionPtr CreateTestT1_2(const KeyRing& keys) {
  Summary summary = Summary::Create();
  Address a = keys.getInnAddr();
  Address w = keys.getWalletAddr(0);
  summary.addItem(a, (uint64_t) 12, (int64_t) -47);
  summary.addItem(w, (uint64_t) 12, (int64_t) 47);
  std::vector<byte> msg = summary.getCanonical();
  auto k = keys.getKey(a);
  auto h = DevcashHash(msg);
  auto s = SignBinary(k, h);
  auto s2 = SignBinary(k, h);
  Tier1Transaction tx1(summary, s, a, keys);
  InputBuffer ib(tx1.getCanonical());
  Tier1Transaction c = Tier1Transaction::Create(ib, keys, true);
  LOG_INFO << "Original addr: " + ToHex(a);
  LOG_INFO << "Final addr: " + ToHex(c.getNodeAddress());

  ib.reset();
  auto d = Tier1Transaction::CreateUniquePtr(ib, keys, true);
  LOG_INFO << "Original ptr addr: " + ToHex(a);
  LOG_INFO << "Final ptr addrp: " + ToHex(d->getNodeAddress());

  return d;
}

TEST_F(Tier1TransactionTest, getJSONIdentity) {
  //size_t offset = 0;
  //uint64_t node_index;
  //bool calculate_soundness = false;
  //Devcash::KeyRing keys(t1_context_0_);

  Transfer sender(keys_.getWalletAddr(1), 0, -1, 0);
  Transfer test_transfer(keys_.getWalletAddr(0), 0, 1, 0);

  std::vector<Transfer> xfers;
  xfers.push_back(sender);
  xfers.push_back(test_transfer);

  EVP_MD_CTX* ctx;
  if(!(ctx = EVP_MD_CTX_create())) {
    LOG_FATAL << "Could not create signature context!";
    exit(-1);
  }

  uint64_t nonce = GetMillisecondsSinceEpoch() + (100);
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce, nonce_bin);
  Tier2Transaction t2_tx1(
      eOpType::Exchange, xfers,
      nonce_bin,
      keys_.getWalletKey(1), keys_);

  auto tx = CreateTestT1_2(keys_);

  /*
   * Summary sum_test = Summary::Create();
  //Devcash::Tier1Transaction identity(sum_test,
                                     t2_tx1.getSignature(),
                                     node_index,
                                     keys);
*/
  std::string stuff;
  EXPECT_EQ(tx->getJSON(), stuff);
}

/*
TEST_F(Tier1TransactionTest, getCanonicalIdentity) {
  Devcash::Transfer test_transfer(addr_0_, 0, 1, 0);
  Devcash::Transfer identity(test_transfer.getCanonical());

  EXPECT_EQ(test_transfer.getCanonical(), identity.getCanonical());
}
*/


/**
 *
 * Tier2TransactionTest
 *
 */
class Tier2TransactionTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  Tier2TransactionTest()
      : t1_context_0_(node_,
                      shard_,
                      Devcash::eAppMode::T1,
                      "", // "../opt/inn.key"
                      "", //"../opt/node.key"
                      "password")
      , transfers_()
      , keys_(t1_context_0_)
  {
    for (int i = 0; i < 4; ++i) {
      keys_.addWalletKeyPair(t1_context_0_.kADDRs[i], t1_context_0_.kADDR_KEYs[i], "password");
    }

    keys_.setInnKeyPair(t1_context_0_.kINN_ADDR, t1_context_0_.kINN_KEY, "password");

    for (int i = 0; i < 3; ++i) {
      keys_.addNodeKeyPair(t1_context_0_.kNODE_ADDRs.at(i), t1_context_0_.kNODE_KEYs.at(i), "password");
    }

    Devcash::Transfer transfer1(keys_.getWalletAddr(0), 0, -10, 0);
    transfers_.push_back(transfer1);
    Devcash::Transfer transfer2(keys_.getWalletAddr(1), 0, 10, 0);
    transfers_.push_back(transfer2);
  }

  // You can do clean-up work that doesn't throw exceptions here.
  ~Tier2TransactionTest()  = default;

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // node
  unsigned int node_ = 1;
  unsigned int shard_ = 2;

  // Create a default context
  Devcash::DevcashContext t1_context_0_;

  // default Transfer list
  std::vector<Devcash::Transfer> transfers_;

  // keys
  Devcash::KeyRing keys_;
};

TEST_F(Tier2TransactionTest, createInnTx_0) {
  size_t addr_count = keys_.CountWallets();
  Address inn_addr = keys_.getInnAddr();

  uint64_t nonce_num = GetMillisecondsSinceEpoch() + 1000011;
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce_num, nonce_bin);

  unsigned int amount = 400000;
  std::vector<Transfer> xfers;
  Transfer inn_transfer(inn_addr, 0, -1 * addr_count * amount, 0);
  xfers.push_back(inn_transfer);
  for (size_t i = 0; i < addr_count; ++i) {
    Transfer transfer(keys_.getWalletAddr(i), 0, amount, 0);
    xfers.push_back(transfer);
  }
  Tier2Transaction inn_tx(eOpType::Create, xfers, nonce_bin,
                          keys_.getKey(inn_addr), keys_);

  EXPECT_EQ(inn_tx.getOperation(), eOpType::Create);
}

TEST_F(Tier2TransactionTest, createInnTx_1) {
  size_t addr_count = keys_.CountWallets();
  Address inn_addr = keys_.getInnAddr();

  uint64_t nonce_num = GetMillisecondsSinceEpoch()/100 + 1000011;
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce_num, nonce_bin);

  std::vector<Transfer> xfers;
  Transfer inn_transfer(inn_addr, 0, -1 * addr_count * (addr_count - 1) * nonce_num, 0);
  xfers.push_back(inn_transfer);
  for (size_t i = 0; i < addr_count; ++i) {
    Transfer transfer(keys_.getWalletAddr(i), 0, (addr_count - 1) * nonce_num, 0);
    xfers.push_back(transfer);
  }
  Tier2Transaction inn_tx(eOpType::Create, xfers, nonce_bin,
                          keys_.getKey(inn_addr), keys_);

  EXPECT_EQ(inn_tx.getOperation(), eOpType::Create);
}

TEST_F(Tier2TransactionTest, getOperation_0) {
  uint64_t nonce_num = 10;
  std::vector<Transfer> peer_xfers;
  Transfer sender(keys_.getWalletAddr(0), 0, nonce_num * -1, 0);
  peer_xfers.push_back(sender);
  Transfer receiver(keys_.getWalletAddr(1), 0, nonce_num, 0);
  peer_xfers.push_back(receiver);

  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce_num, nonce_bin);

  Tier2Transaction tx1(
      eOpType::Exchange,
      peer_xfers,
      nonce_bin,
      keys_.getWalletKey(0),
      keys_);

  EXPECT_EQ(tx1.getOperation(), eOpType::Exchange);
}

TEST_F(Tier2TransactionTest, op2_0) {
  std::vector<byte> nonce_bin;
  uint64_t nonce_num = GetMillisecondsSinceEpoch() + (1000000);
  Uint64ToBin(nonce_num, nonce_bin);
  Tier2Transaction tx1(
              eOpType::Exchange,
              transfers_,
              nonce_bin,
              keys_.getWalletKey(0),
              keys_);

  EXPECT_EQ(tx1.getByteSize(), 215);
}


TEST_F(Tier2TransactionTest, clone_0) {
  std::vector<byte> nonce_bin;
  uint64_t nonce_num = GetMillisecondsSinceEpoch();
  Uint64ToBin(nonce_num, nonce_bin);
  Tier2Transaction tx1(
              eOpType::Exchange,
              transfers_,
              nonce_bin,
              keys_.getWalletKey(0),
              keys_);

  auto tx2ptr = tx1.clone();

  EXPECT_EQ(tx1, *tx2ptr);
}

TEST_F(Tier2TransactionTest, getNonce_0) {
  uint64_t nonce = 12345678;
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce, nonce_bin);
  Tier2Transaction tx1(
              eOpType::Exchange,
              transfers_,
              nonce_bin,
              keys_.getWalletKey(0),
              keys_);
  size_t offset = 0;
  uint64_t nonce_copy = BinToUint64(tx1.getNonce(), offset);
  EXPECT_EQ(nonce, nonce_copy);
}

TEST_F(Tier2TransactionTest, getBigNonce_0) {
  std::vector<byte> nonce_bin;
  for (int i=0; i<256; ++i) {
    nonce_bin.push_back((byte) i);
  }
  Tier2Transaction tx1(
              eOpType::Exchange,
              transfers_,
              nonce_bin,
              keys_.getWalletKey(0),
              keys_);
  EXPECT_EQ(nonce_bin, tx1.getNonce());
}

TEST_F(Tier2TransactionTest, isSound_0) {
  uint64_t nonce = 12345678;
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce, nonce_bin);
  Tier2Transaction tx1(
              eOpType::Exchange,
              transfers_,
              nonce_bin,
              keys_.getWalletKey(0),
              keys_);

  EXPECT_TRUE(tx1.isSound(keys_));
}

TEST_F(Tier2TransactionTest, checkCanonical) {
  uint64_t nonce = 12345678;
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce, nonce_bin);
  Tier2Transaction tx1(
              eOpType::Exchange,
              transfers_,
              nonce_bin,
              keys_.getWalletKey(0),
              keys_);

  EXPECT_EQ(116, BinToUint64(tx1.getCanonical(), 0));
}

TEST_F(Tier2TransactionTest, serdes_0) {
  uint64_t nonce = 123456789;
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce, nonce_bin);
  Tier2Transaction tx1(
              eOpType::Exchange,
              transfers_,
              nonce_bin,
              keys_.getWalletKey(0),
              keys_);

  InputBuffer buffer(tx1.getCanonical());

  Tier2Transaction tx2 = Tier2Transaction::Create(buffer, keys_, true);
  EXPECT_EQ(tx1, tx2);
}

TEST_F(Tier2TransactionTest, serdes_1) {
  uint64_t nonce = GetMillisecondsSinceEpoch() + (100);
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce, nonce_bin);
  Tier2Transaction tx1(
              eOpType::Exchange, transfers_,
              nonce_bin,
              keys_.getWalletKey(0), keys_);

  InputBuffer buffer(tx1.getCanonical());

  auto tx2 = Tier2Transaction::CreateUniquePtr(buffer, keys_, true);
  EXPECT_EQ(tx1, *tx2);
}

TEST(Primitives, getCanonical0) {
  DevcashContext this_context(1,
                      1,
                      Devcash::eAppMode::T1,
                      "",
                      "",
                      "");

  KeyRing keys(this_context);

  Transfer test_transfer(keys.getWalletAddr(0), 0, 1, 0);
  InputBuffer buffer(test_transfer.getCanonical());
  Transfer identity(buffer);

  EXPECT_EQ(test_transfer.getJSON(), identity.getJSON());
  EXPECT_EQ(test_transfer.getCanonical(), identity.getCanonical());

  Transfer sender(keys.getWalletAddr(1), 0, -1, 0);

  std::vector<Transfer> xfers;
  xfers.push_back(sender);
  xfers.push_back(test_transfer);

  EVP_MD_CTX* ctx;
  if(!(ctx = EVP_MD_CTX_create())) {
    LOG_FATAL << "Could not create signature context!";
    exit(-1);
  }

  uint64_t nonce = GetMillisecondsSinceEpoch() + (100);
  std::vector<byte> nonce_bin;
  Uint64ToBin(nonce, nonce_bin);
  Tier2Transaction tx1(
              eOpType::Exchange, xfers,
              nonce_bin,
              keys.getWalletKey(1), keys);

  InputBuffer buffer2(tx1.getCanonical());

  Tier2Transaction tx2 = Tier2Transaction::Create(buffer2, keys, true);

  EXPECT_EQ(tx1, tx2);
  EXPECT_EQ(tx1.getCanonical(), tx2.getCanonical());
  EXPECT_EQ(tx1.getJSON(), tx2.getJSON());

  Validation val_test = Validation::Create();
  val_test.addValidation(keys.getNodeAddr(0), tx1.getSignature());
  InputBuffer buffer3(val_test.getCanonical());
  Validation val_test2 = Validation::Create(buffer3);

  EXPECT_EQ(val_test.getCanonical(), val_test2.getCanonical());
  EXPECT_EQ(GetJSON(val_test), GetJSON(val_test2));

  Summary sum_test = Summary::Create();
  DelayedItem delayed(0, 1);
  sum_test.addItem(keys.getWalletAddr(0), (uint64_t) 0, delayed);
  InputBuffer buffer4(sum_test.getCanonical());
  Summary sum_identity = Summary::Create(buffer4);

  EXPECT_EQ(sum_test.getCanonical(), sum_identity.getCanonical());
  EXPECT_EQ(GetJSON(sum_test), GetJSON(sum_identity));

  TransactionCreationManager txm(eAppMode::T2);
  Hash prev_hash;
  buffer2.reset();
  auto txptr = Tier2Transaction::CreateUniquePtr(buffer2, keys);
  //std::unique_ptr<Transaction> a(txptr.release());
  std::vector<TransactionPtr> txs;
  txs.push_back(std::move(txptr));
  ChainState state;
  ProposedBlock proposal_test(prev_hash, txs, sum_test, val_test, state);
  InputBuffer buffer5(proposal_test.getCanonical());
  ProposedBlock proposal_id = ProposedBlock::Create(buffer5, state, keys, txm);

  EXPECT_EQ(proposal_test.getCanonical(), proposal_id.getCanonical());
  EXPECT_EQ(GetJSON(proposal_test), GetJSON(proposal_id));

  FinalBlock final_test(proposal_test);
  InputBuffer buffer6(final_test.getCanonical());
  FinalBlock final_id(buffer6, proposal_test.getBlockState(), keys, eAppMode::T2);

  EXPECT_EQ(GetJSON(final_test), GetJSON(final_id));
  EXPECT_EQ(final_test.getCanonical(), final_id.getCanonical());
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
