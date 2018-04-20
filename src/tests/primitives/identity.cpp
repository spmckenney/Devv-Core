#include <string>

#include "common/argument_parser.h"
#include "common/devcash_context.h"
#include "common/logger.h"
#include "common/ossladapter.h"
#include "common/util.h"
#include "primitives/Transfer.h"
#include "primitives/Transaction.h"

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

  LOG_INFO << "Binr: "+toHex(test_transfer.getCanonical());
  LOG_INFO << "Test: "+test_transfer.getJSON();
  LOG_INFO << "Idnt: "+identity.getJSON();

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
    CASH_THROW("Could not create signature context!");
  }

  EC_KEY* inn_key = loadEcKey(ctx,
	this_context.kINN_ADDR,
	this_context.kINN_KEY);

  Transaction tx_test(eOpType::Exchange, xfers, (uint64_t) 0, inn_key);
  Transaction tx_identity(tx_test.getCanonical());

  if (tx_test.getJSON() == tx_identity.getJSON()) {
    LOG_INFO << "Transaction JSON identity pass.";
  }

  if (tx_test.getCanonical() == tx_identity.getCanonical()) {
    LOG_INFO << "Transaction canonical identity pass.";
  }
  exit(0);
}