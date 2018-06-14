/*
 * KeyRing.h definted key management for Devcash.
 *
 *  Created on: Mar 3, 2018
 *      Author: Nick Williams
 */

#ifndef CONSENSUS_KEYRING_H_
#define CONSENSUS_KEYRING_H_

#include "common/devcash_context.h"
#include "primitives/Transfer.h"

namespace Devcash {


class KeyRing {
 public:
  KeyRing(const DevcashContext& context);
  virtual ~KeyRing() {};

  /**
   * Add an address and EC key to this directory.
   * @param hex - a hexadecimal representation of this address.
   * @param key - a pointer to the openssl EC_KEY for this address
   * @return a binary representation of the provided address
   */
  Address InsertAddress(std::string hex, EC_KEY* key);
  /**
   * Get a pointer to the EC key corresponding to an address.
   * @param addr - the Address to obtain the key for
   * @return a pointer to the openssl EC key for the provided address
   */
  EC_KEY* getKey(const Address& addr) const;
  /**
   * Check if the provided address belongs to the INN space
   * @param addr - the Address to check
   * @return true, iff this Address is an INN Address
   */
  bool isINN(const Address& addr) const;
  /**
   * @return the default INN Address
   */
  Address getInnAddr() const;
  /**
   * @return the number of node keys and addresses in this directory.
   */
  int CountNodes() const;
  /**
   * @return the number of wallet keys and addresses in this directory.
   */
  int CountWallets() const;
  /**
   * @param addr - the Address to check
   * @return the index of the provided node address
   * @return -1 if this Address is not a known node in this directory
   */
  int getNodeIndex(const Address& addr) const;
  /**
   * @param index - the index of the node to retrieve
   * @return the Address of the node at the provided index
   * @throw std::out_of_range if there is not a node at the given index
   */
  Address getNodeAddr(size_t index) const;
  /**
   * @param index - the index of the wallet to retrieve
   * @return the Address of the wallet at the provided index
   * @throw std::out_of_range if there is not a wallet at the given index
   */
  Address getWalletAddr(int index) const;
  /**
   * @param index - the index of the node to retrieve
   * @return the EC key of the node at the provided index
   * @throw std::out_of_range if there is not a node at the given index
   */
  EC_KEY* getNodeKey(int index) const;
  /**
   * @param index - the index of the wallet to retrieve
   * @return the EC key of the wallet at the provided index
   * @throw std::out_of_range if there is not a wallet at the given index
   */
  EC_KEY* getWalletKey(int index) const;
  /**
   * Retrieve the list of wallets managed by a particular node.
   * @param index - the index of the node to retrieve wallet addresses for
   * @return a vector of wallet Addresses managed by the node at the given index
   * @throw std::out_of_range if there is not a node at the given index
   */
  std::vector<Address> getDesignatedWallets(int index) const;

 private:
  KeyRing();
  KeyRing& operator=(KeyRing&);
  KeyRing(KeyRing&);

  DevcashContext context_;
  std::map<Address, EC_KEY*> key_map_;
  std::vector<Address> node_list_;
  std::vector<Address> wallet_list_;
  Address inn_addr_;
};

} /* namespace Devcash */

#endif /* CONSENSUS_KEYRING_H_ */
