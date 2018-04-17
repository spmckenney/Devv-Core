/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include "flatbuffers/devcash_primitives_generated.h"
#include "flatbuffers/fbs_strategy.h"
#include "flatbuffers/FbsTransfer.h"

namespace Devcash
{

template <>
class DataManager<FlatBufferStrategy> {
public:
  typedef Devcash::fbs::SummaryItem SummaryItemType;
  typedef Devcash::fbs::Summary SummaryType;
  typedef Devcash::fbs::Transaction TransactionType;
  typedef Devcash::Transfer<FlatBufferStrategy> TransferType;

  //Devcash::FinalBlockManagerSharedPtr CreateFinalBlock();

  TransferType CreateTransfer(Buffer address,
                              int64_t amount,
                              int64_t coin_index,
                              int64_t delay) {
    return(TransferType(fbs::CreateTransfer(builder_,
                                            address,
                                            amount,
                                            coin_index,
                                            delay)));
  }
  /**
   * Creates an initialized SummaryItem.
   */
    /*
  Devcash::SummaryItem* CreateSummaryItem(uint8_t delta,
                                          uint8_t delay) {

    auto summary_item = Primitives::CreateSummaryItem(*builder_
                                                      , 1
                                                      , 2);
    return(summary_item);
  }
    */

  /**
   * Creates an initialized Summary object
   */
  //Devcash::Summary* CreateSummary() {}

private:
  flatbuffers::FlatBufferBuilder builder_;
};

} // namespace Devcash
