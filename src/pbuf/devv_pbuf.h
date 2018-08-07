/*
 * ${Filename}
 *
 *  Created on: 8/6/18
 *      Author: mckenney
 */
#ifndef DEVCASH_DEVV_PBUF_H
#define DEVCASH_DEVV_PBUF_H

#include <vector>

#include "primitives/Tier2Transaction.h"

#include "devv.pb.h"

namespace Devcash {

Tier2TransactionPtr GetT2TxFromProtobufString(const std::string& pb_tx, const KeyRing& keys) {

  proto::Transaction tx;
  tx.ParseFromString(pb_tx);

  auto pb_xfers = tx.xfers();

  std::vector<Transfer> transfers;
  EC_KEY* key = nullptr;
  for (auto const& xfer : pb_xfers) {
    std::vector<byte> bytes(xfer.address().begin(), xfer.address().end());
    auto address = Address(bytes);
    transfers.emplace_back(address, xfer.coin(), xfer.amount(), xfer.delay());
    if (xfer.amount() < 0) {
      if (key != nullptr) {
        // WOOP WOOP
      }
      key = keys.getKey(address);
    }
  }

  if (key == nullptr) {
    // WOOP WOOP
  }

  std::vector<byte> nonce(tx.nonce().begin(), tx.nonce().end());

  Tier2TransactionPtr t2tx_ptr = std::make_unique<Tier2Transaction>(
      eOpType::Exchange,
      transfers,
      nonce,
      key,
      keys);

  return(t2tx_ptr);
}

} // namespace Devcash

#endif //DEVCASH_DEVV_PBUF_H
