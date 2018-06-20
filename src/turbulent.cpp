/*
 * turbulent.cpp
 * Creates generate_count transactions as follows:
 * 1.  INN transactions create coins for every address
 * 2.  Each peer address attempts to send a random number of coins up to tx_limit
 *     to a random address other than itself
 * 3.  Many transactions will be invalid, but valid transactions should also appear indefinitely
 *
 *  Created on: May 24, 2018
 *      Author: Nick Williams
 */

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include "common/argument_parser.h"
#include "common/devcash_context.h"
#include "node/DevcashNode.h"

using namespace Devcash;

#define UNUSED(x) ((void)x)

int main(int argc, char* argv[]) {
  init_log();

  CASH_TRY {
    std::unique_ptr<devcash_options> options = parse_options(argc, argv);

    if (!options) {
      exit(-1);
    }

    DevcashContext this_context(options->node_index, options->shard_index, options->mode, options->inn_keys,
                                options->node_keys, options->wallet_keys, options->sync_port, options->sync_host);

    KeyRing keys(this_context);

    std::vector<byte> out;
    EVP_MD_CTX* ctx;
    if (!(ctx = EVP_MD_CTX_create())) {
      LOG_FATAL << "Could not create signature context!";
      CASH_THROW("Could not create signature context!");
    }

    Address inn_addr = keys.getInnAddr();

    size_t addr_count = keys.CountWallets();

    std::srand((unsigned)time(0));
    size_t counter = 0;
    size_t batch_counter = 0;

    std::vector<Transfer> xfers;
    Transfer inn_transfer(inn_addr, 0, -1 * addr_count * options->tx_limit, 0);
    xfers.push_back(inn_transfer);
    for (size_t i = 0; i < addr_count; ++i) {
      Transfer transfer(keys.getWalletAddr(i), 0, options->tx_limit, 0);
      xfers.push_back(transfer);
    }
    Tier2Transaction inn_tx(eOpType::Create, xfers, GetMillisecondsSinceEpoch() +
                            (1000000 * (options->node_index + 1) * (batch_counter + 1)),
                            keys.getKey(inn_addr), keys);
    std::vector<byte> inn_canon(inn_tx.getCanonical());
    out.insert(out.end(), inn_canon.begin(), inn_canon.end());
    LOG_DEBUG << "GenerateTransactions(): generated inn_tx with sig: " << ToHex(inn_tx.getSignature());
    counter++;

    while (counter < options->generate_count) {
      while (batch_counter < options->tx_batch_size) {
        for (size_t i = 0; i < addr_count; ++i) {
          size_t j = std::rand() % addr_count;
          size_t amount = std::rand() % options->tx_limit;
          if (i == j) continue;
          std::vector<Transfer> peer_xfers;
          Transfer sender(keys.getWalletAddr(i), 0, amount * -1, 0);
          peer_xfers.push_back(sender);
          Transfer receiver(keys.getWalletAddr(j), 0, amount, 0);
          peer_xfers.push_back(receiver);
          Tier2Transaction peer_tx(
              eOpType::Exchange, peer_xfers,
              GetMillisecondsSinceEpoch() + (1000000 * (options->node_index + 1) * (i + 1) * (j + 1)),
              keys.getWalletKey(i), keys);
          std::vector<byte> peer_canon(peer_tx.getCanonical());
          out.insert(out.end(), peer_canon.begin(), peer_canon.end());
          LOG_TRACE << "GenerateTransactions(): generated tx with sig: " << ToHex(peer_tx.getSignature());
          batch_counter++;
          if (batch_counter >= options->tx_batch_size
              || (counter+batch_counter) >= options->generate_count) break;
        }  // end outer for
        if (batch_counter >= options->tx_batch_size
            || (counter+batch_counter) >= options->generate_count) break;
      }  // end batch while
      counter += batch_counter;
      batch_counter = 0;
    }  // end counter while

    LOG_INFO << "Generated " << counter << " transactions.";

    if (!options->write_file.empty()) {
      std::ofstream outFile(options->write_file, std::ios::out | std::ios::binary);
      if (outFile.is_open()) {
        outFile.write((const char*)out.data(), out.size());
        outFile.close();
      } else {
        LOG_FATAL << "Failed to open output file '" << options->write_file << "'.";
        return (false);
      }
    }

    return (true);
  }
  CASH_CATCH(...) {
    std::exception_ptr p = std::current_exception();
    std::string err("");
    err += (p ? p.__cxa_exception_type()->name() : "null");
    LOG_FATAL << "Error: " + err << std::endl;
    std::cerr << err << std::endl;
    return (false);
  }
}
