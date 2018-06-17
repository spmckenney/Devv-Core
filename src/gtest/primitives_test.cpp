//
// Created by mckenney on 6/11/18.
//

#include "gtest/gtest.h"

#include "primitives/Summary.h"
#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"
#include "primitives/Transfer.h"

namespace {

#define TEST_DESCRIPTION(desc) RecordProperty("description", desc)

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

  ~InputBufferTest()  = default;

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

TEST_F(InputBufferTest, test_getOffsetRef) {
  byte int0 = 1;
  byte int1 = 8;
  std::vector<byte> b1;
  b1.push_back(int0);
  b1.push_back(int1);

  InputBuffer ibuffer(b1);

  byte check;
  size_t& offset = ibuffer.getOffsetRef();
  offset++;
  check = ibuffer.getNextByte();
  EXPECT_EQ(int1, check);
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
  SummaryTest() : t1_context_0_(0, 0, Devcash::eAppMode::T1, "", "", "", 0)
      , addr_0_()
  {
    std::vector<Devcash::byte> tmp(Devcash::Hex2Bin(t1_context_0_.kADDRs[0]));
    std::copy_n(tmp.begin(), Devcash::kADDR_SIZE, addr_0_.begin());
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
  size_t bytes_per_item = 69;
  s.addItem(addr_0_, 10, 10);
  EXPECT_EQ(s.getByteSize(), bytes_per_item);
}

TEST_F(SummaryTest, addItem_1) {
  TEST_DESCRIPTION("Tests addItem() with delay and checks size");
  Summary s = Summary::Create();

  /// @todo (mckenney) maybe this should be calculated instead of being
  /// a magic number
  size_t bytes_per_item = 77;
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

  size_t bytes_per_item = 77;
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

  size_t bytes_per_item = 69;
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

  size_t size0 = 69;
  size_t size1 = 69;
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

  size_t size0 = 69;
  size_t size1 = 69;
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

  size_t size0 = 69;
  size_t size1 = 85;
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
  TransferTest() : t1_context_0_(0, 0, Devcash::eAppMode::T1, "", "", "", 0)
      , addr_0_()
  {
    std::vector<Devcash::byte> tmp(Devcash::Hex2Bin(t1_context_0_.kADDRs[0]));
    std::copy_n(tmp.begin(), Devcash::kADDR_SIZE, addr_0_.begin());
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

TEST_F(TransferTest, Size) {
  EXPECT_EQ(Devcash::Transfer::Size(), Devcash::kADDR_SIZE + 24);
}

TEST_F(TransferTest, getAddress_0) {
  std::vector<Devcash::byte> tmp(33, 'A');
  auto a = Devcash::ConvertToAddress(tmp);
  Devcash::Transfer test0(a, 0, 1, 0);
  EXPECT_EQ(test0.getAddress(), a);
}

TEST_F(TransferTest, getAddress_1) {
  Devcash::Transfer test0(addr_0_, 0, 1, 0);
  std::vector<Devcash::byte> tmp(Devcash::Hex2Bin(t1_context_0_.kADDRs[0]));
  EXPECT_EQ(test0.getAddress(), Devcash::ConvertToAddress(tmp));
}

// Exception - wrong size input
TEST_F(TransferTest, getAddress_2) {
  Devcash::Transfer test0(addr_0_, 0, 1, 0);
  Devcash::Address addr;
  std::vector<Devcash::byte> tmp(Devcash::Hex2Bin(""));
  std::copy_n(tmp.begin(), Devcash::kADDR_SIZE, addr.begin());
  EXPECT_THROW(Devcash::ConvertToAddress(tmp), std::runtime_error);
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

  Tier1TransactionTest() : t1_context_0_(0, 0, Devcash::eAppMode::T1, "", "", "", 0)
      , addr_0_()
  {
    std::vector<Devcash::byte> tmp(Devcash::Hex2Bin(t1_context_0_.kADDRs[0]));
    std::copy_n(tmp.begin(), Devcash::kADDR_SIZE, addr_0_.begin());
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

  // create a default address
  Devcash::Address addr_0_;

};

TEST_F(Tier1TransactionTest, defaultConstructor) {
  size_t zero = 0;
  Devcash::Tier1Transaction transaction;

  // @todo (mckenney) should be Devcash::Tier1Transaction::MinSize()?
  EXPECT_EQ(transaction.getByteSize(), zero);
}

/*
TEST_F(Tier1TransactionTest, getJSONIdentity) {
  size_t offset = 0;
  bool calculate_soundness = false;
  Devcash::KeyRing rings(t1_context_0_);
  Devcash::Tier1Transaction transaction;
  Devcash::Tier1Transaction identity(transaction.getCanonical(), offset, rings, calculate_soundness);

  EXPECT_EQ(transaction.getJSON(), transaction.getJSON());
}
 */
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
      : t1_context_0_(0, 0, Devcash::eAppMode::T1, "", "", "", 0)
      , addr_0_()
      , transfers_()
      , rings_(t1_context_0_)
  {
    std::vector<Devcash::byte> tmp(Devcash::Hex2Bin(t1_context_0_.kADDRs[0]));
    std::copy_n(tmp.begin(), Devcash::kADDR_SIZE, addr_0_.begin());

    Devcash::Transfer transfer(addr_0_, 0, 1, 2);
    transfers_.push_back(transfer);
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

  // Objects declared here can be used by all tests in the test case for Foo.

  // Create a default context
  Devcash::DevcashContext t1_context_0_;

  // create a default address
  Devcash::Address addr_0_;

  // default Transfer list
  std::vector<Devcash::Transfer> transfers_;

  // rungs
  Devcash::KeyRing rings_;
};
/*
TEST_F(Tier2TransactionTest, constructor_0) {
  size_t zero = 0;
  Devcash::Signature sig;
  Devcash::Tier2Transaction transaction(10, transfers_, 10, sig, rings_);

  // @todo (mckenney) should be Devcash::Tier1Transaction::MinSize()?
  EXPECT_EQ(transaction.getByteSize(), zero);
}
*/
/*
TEST_F(Tier1TransactionTest, getJSONIdentity) {
  size_t offset = 0;
  bool calculate_soundness = false;
  Devcash::KeyRing rings(t1_context_0_);
  Devcash::Tier1Transaction transaction;
  Devcash::Tier1Transaction identity(transaction.getCanonical(), offset, rings, calculate_soundness);

  EXPECT_EQ(transaction.getJSON(), transaction.getJSON());
}
 */

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}