/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

namespace Devcash {

template <typename DataManagerImpl>
class DataManager {
public:
  typedef typename DataManagerImpl::Buffer Buffer;
  DataManagerImpl GetDataManager();

  typename DataManagerImpl::TransferType CreateTransfer(Buffer address,
                                                        int64_t amount,
                                                        int64_t coin_index,
                                                        int64_t delay) {
    return(data_manager_.CreateTransfer(address,
                                        amount,
                                        coin_index,
                                        delay));
  }

private:
  DataManagerImpl data_manager_;
};

typedef int8_t Byte;
typedef std::vector<Byte> Buffer;

} // namespace Devcash
