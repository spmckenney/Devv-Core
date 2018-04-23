#include <string>

#include "common/argument_parser.h"
#include "common/devcash_context.h"
#include "common/logger.h"
#include "common/ossladapter.h"
#include "common/util.h"
#include "primitives/FinalBlock.h"

int main(int argc, char* argv[])
{
  init_log();
  std::unique_ptr<devcash_options> options = parse_options(argc, argv);

  if (!options) {
    exit(-1);
  }

  DevcashContext this_context(options->node_index,
                              static_cast<eAppMode>(options->mode));
  std::vector<byte> tmp(Hex2Bin(this_context.kADDRs[0]));
  Address addr_0;
  std::copy_n(tmp.begin(), kADDR_SIZE, addr_0.begin());
  Transfer test_transfer(addr_0, 0, 1, 0);
  Transfer identity(test_transfer.getCanonical());

  if (test_transfer.getJSON() == identity.getJSON()) {
    LOG_INFO << "Transfer JSON identity pass.";
  }

  if (test_transfer.getCanonical() == identity.getCanonical()) {
    LOG_INFO << "Transfer canonical identity pass.";
  }

  Address addr_1;
  std::vector<byte> tmp2(Hex2Bin(this_context.kADDRs[1]));
  std::copy_n(tmp2.begin(), kADDR_SIZE, addr_1.begin());
  Transfer sender(addr_1, 0, -1, 0);

  std::vector<Transfer> xfers;
  xfers.push_back(test_transfer);
  xfers.push_back(sender);

  EVP_MD_CTX* ctx;
  if(!(ctx = EVP_MD_CTX_create())) {
    LOG_FATAL << "Could not create signature context!";
    exit(-1);
  }

  EC_KEY* addr_key = loadEcKey(ctx,
      this_context.kADDRs[1],
      this_context.kADDR_KEYs[1]);

  KeyRing keys(this_context);
  keys.initKeys();

  Transaction tx_test(eOpType::Exchange, xfers, (uint64_t) 1, addr_key, keys);
  Transaction tx_identity(tx_test.getCanonical(), keys);

  if (tx_test.getJSON() == tx_identity.getJSON()) {
    LOG_INFO << "Transaction JSON identity pass.";
  }

  if (tx_test.getCanonical() == tx_identity.getCanonical()) {
    LOG_INFO << "Transaction canonical identity pass.";
  }

  Address node_1;
  std::vector<byte> tmp3(Hex2Bin(this_context.kNODE_ADDRs[0]));
  std::copy_n(tmp3.begin(), kADDR_SIZE, node_1.begin());

  Validation val_test(node_1, tx_test.getSignature());
  size_t offset = 0;
  uint32_t val_count = val_test.GetValidationCount();
  Validation val_identity(val_test.getCanonical(), offset
      , val_count);

  if (val_test.getJSON() == val_identity.getJSON()) {
    LOG_INFO << "Validation JSON identity pass.";
  }

  if (val_test.getCanonical() == val_identity.getCanonical()) {
    LOG_INFO << "Validation canonical identity pass.";
  }

  Summary sum_test;
  DelayedItem delayed(0, 1);
  sum_test.addItem(addr_1, (uint64_t) 0, delayed);
  Summary sum_identity(sum_test);
  if (sum_test.getJSON() == sum_identity.getJSON()) {
    LOG_INFO << "Summary JSON identity pass.";
  }

  if (sum_test.getCanonical() == sum_identity.getCanonical()) {
    LOG_INFO << "Summary canonical identity pass.";
  }

  Hash prev_hash;
  std::vector<Transaction> txs;
  txs.push_back(tx_test);
  ChainState state;
  ProposedBlock proposal_test(prev_hash, txs, sum_test, val_test, state);
  ProposedBlock proposal_id(proposal_test.getCanonical(), state, keys);

  if (proposal_test.getJSON() == proposal_id.getJSON()) {
    LOG_INFO << "Proposal Block JSON identity pass.";
  }

  if (proposal_test.getCanonical() == proposal_id.getCanonical()) {
    LOG_INFO << "Proposal Block canonical identity pass.";
  }

  FinalBlock final_test(proposal_test);
  FinalBlock final_id(final_test.getCanonical()
      , proposal_test.getBlockState(), keys);

  if (final_test.getJSON() == final_id.getJSON()) {
    LOG_INFO << "Final Block JSON identity pass.";
  }

  if (final_test.getCanonical() == final_id.getCanonical()) {
    LOG_INFO << "Final Block canonical identity pass.";
  }

  exit(0);
}