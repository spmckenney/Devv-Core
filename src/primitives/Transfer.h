/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#include <primitives/strategy.h>

struct TransferImpl {
  std::vector<int8_t> addr;
  int64_t amount;
  int64_t coin_index;
  int64_t delay;
};

struct BasicStrategy {
  typedef TransferImpl TransferType;
}{
/**
 * Transfer
 */
template <typename StorageManager, typename StorageStrategy>
class Transfer {
public:
  // Type of this class
  typedef StorageManager<StorageStrategy>::TransferType TransferType;

  // Type of implementation struct
  typedef StorageManager<StorageStrategy>::TransferTypeImpl TransferTypeImpl;

  /**
   * Constructor
   */
  void Transfer(StorageManager::Buffer address,
                int64_t amount,
                int64_t coin_index,
                int64_t delay);

  StorageManager::Buffer address();

  void set_address(Buffer address) {
    transfer_.set_address(address);
  }

  int64_t amount() {
    return(transfer_.amount());
  }

  void set_amount(int64_t amount) {
    transfer_.set_amount(amount);
  }

  int64_t coin_index() {
    return(transfer_.coin_index());
  }

  void set_coin_index(int64_t coin_index) {
    transfer_.set_coin_index(coin_index);
  }

  int64_t delay() {
    return(transfer_.delay());
  }

  void set_delay(int64_t delay) {
    transfer_.set_delay(delay);
  }

  std::string GetCanonical() const {
  }

  /** Compare transfers */
  friend bool operator==(const TransferType& a, const TransferType& b)
  {
      return (a.transfer_ = b.transfer_);
  }

  friend bool operator!=(const TransferType& a, const TransferType& b)
  {
    return !(a.transfer_ == b.transfer_);
  }

  void SetNull() {
    transfer_.set_address("");
    transver_.set_amount = 0;
  }

  void IsNull() {
    return(transfer_.get_address() == "");
  }

private:
  TransferTypeImpl transfer_;
};
