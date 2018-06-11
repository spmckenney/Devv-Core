//
// Created by mckenney on 6/11/18.
//

#include "primitives/Transfer.h"
#include "gtest/gtest.h"

namespace {

// The fixture for testing class Foo.
class TransferTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  TransferTest() :
  t1_context_0(0, 0, Devcash::eAppMode::T1, "", "", "", 0) {
    // You can do set-up work for each test here.
  }

  ~TransferTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

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
  Devcash::DevcashContext t1_context_0;
};

// Tests that the Foo::Bar() method does Abc.
TEST_F(TransferTest, getJSONIdentity) {
  std::vector<Devcash::byte> tmp(Devcash::Hex2Bin(t1_context_0.kADDRs[0]));
  Devcash::Address addr_0;
  std::copy_n(tmp.begin(), Devcash::kADDR_SIZE, addr_0.begin());
  Devcash::Transfer test_transfer(addr_0, 0, 1, 0);
  Devcash::Transfer identity(test_transfer.getCanonical());

  EXPECT_EQ(test_transfer.getJSON(), identity.getJSON());
}

// Tests that Foo does Xyz.
TEST_F(TransferTest, getCanonical) {
  // Exercises the Xyz feature of Foo.
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}