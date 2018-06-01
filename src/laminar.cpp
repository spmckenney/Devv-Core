/*
 * laminar.cpp
 * Creates up to generate_count transactions as follows:
 * 1.  INN transactions create addr_count coins for every address
 * 2.  Each peer address attempts to send tx_limit coins to every higher address
 * 3.  If invalid transactions are dropped and the generate_count is high enough
 *     , all coins end at the highest address
 *
 *  Created on: May 24, 2018
 *      Author: Nick Williams
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include "common/argument_parser.h"
#include "common/devcash_context.h"
#include "node/DevcashNode.h"

using namespace Devcash;

#define UNUSED(x) ((void)x)

int main(int argc, char* argv[])
{

  init_log();

  CASH_TRY {
    std::unique_ptr<devcash_options> options = parse_options(argc, argv);

    if (!options) {
      exit(-1);
    }

    DevcashContext this_context(options->node_index
      , options->shard_index
      , options->mode
      , options->inn_keys
      , options->node_keys
      , options->wallet_keys
      , options->sync_port
      , options->sync_host);

    KeyRing keys(this_context);

    std::vector<byte> out;
    EVP_MD_CTX* ctx;
    if(!(ctx = EVP_MD_CTX_create())) {
      LOG_FATAL << "Could not create signature context!";
      CASH_THROW("Could not create signature context!");
    }

    Address inn_addr = keys.getInnAddr();

    if (options->generate_count < 2) {
      LOG_FATAL << "Must generate at least 2 transactions for a complete circuit.";
      CASH_THROW("Invalid number of transactions to generate.");
    }
    //Need sqrt(N-1) addresses (x) to create N circuits: 1+x(x-1)+x=N
    double need_addrs = std::sqrt(options->generate_count-1);
    //if sqrt(N-1) is not an int, circuits will be incomplete
    if (std::floor(need_addrs) != need_addrs) {
      LOG_WARNING << "For complete circuits generate a perfect square + 1 transactions (ie 2,5,10,17...)";
    }
    size_t addr_count = std::min(keys.CountWallets()
      , static_cast<unsigned int>need_addrs);

    size_t counter = 0;
    size_t batch_counter = 0;
    while (counter < options->generate_count) {
      while (batch_counter < options->tx_batch_size) {
        std::vector<Transfer> xfers;
        Transfer inn_transfer(inn_addr, 0, -1*addr_count, 0);
        xfers.push_back(inn_transfer);
        for (size_t i=0; i<addr_count; ++i) {
          Transfer transfer(keys.getWalletAddr(i), 0, options->tx_limit, 0);
          xfers.push_back(transfer);
        }
        Tier2Transaction inn_tx(eOpType::Create, xfers
                           , getEpoch()+(1000000*(options->node_index+1)*(batch_counter+1))
            , keys.getKey(inn_addr), keys);
        std::vector<byte> inn_canon(inn_tx.getCanonical());
        out.insert(out.end(), inn_canon.begin(), inn_canon.end());
        LOG_DEBUG << "GenerateTransactions(): generated inn_tx with sig: " << toHex(inn_tx.getSignature());
        batch_counter++;
        for (size_t i=0; i<addr_count; ++i) {
          for (size_t j=i; j<addr_count; ++j) {
            if (i==j) continue;
            std::vector<Transfer> peer_xfers;
            Transfer sender(keys.getWalletAddr(i), 0, options->tx_limit*-1, 0);
            peer_xfers.push_back(sender);
            Transfer receiver(keys.getWalletAddr(j), 0, options->tx_limit, 0);
            peer_xfers.push_back(receiver);
            Tier2Transaction peer_tx(eOpType::Exchange, peer_xfers
                                , getEpoch()+(1000000*(options->node_index+1)*(i+1)*(j+1))
                                , keys.getWalletKey(i), keys);
            std::vector<byte> peer_canon(peer_tx.getCanonical());
            out.insert(out.end(), peer_canon.begin(), peer_canon.end());
            LOG_TRACE << "GenerateTransactions(): generated tx with sig: " << toHex(peer_tx.getSignature());
            batch_counter++;
            if (batch_counter >= options->tx_batch_size) break;
          } //end inner for
          if (batch_counter >= options->tx_batch_size) break;
        } //end outer for
        if (batch_counter >= options->tx_batch_size) break;
      } //end batch while
      counter += batch_counter;
      batch_counter = 0;
    } //end counter while

    LOG_INFO << "Generated " << counter << " transactions.";

    if (!options->write_file.empty()) {
      std::ofstream outFile(options->write_file
          , std::ios::out | std::ios::binary);
      if (outFile.is_open()) {
        outFile.write((const char*) out.data(), out.size());
        outFile.close();
      } else {
        LOG_FATAL << "Failed to open output file '" << options->write_file << "'.";
        return(false);
      }
    }

    return(true);
  } CASH_CATCH (...) {
    std::exception_ptr p = std::current_exception();
    std::string err("");
    err += (p ? p.__cxa_exception_type()->name() : "null");
    LOG_FATAL << "Error: "+err <<  std::endl;
    std::cerr << err << std::endl;
    return(false);
  }
}
