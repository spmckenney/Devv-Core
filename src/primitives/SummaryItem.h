/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#include "primitives/fbs_strategy.h"

template <BufferManager>
class SummaryItem {
public:
  int64_t get_delta() const
  {
    return summary_item_->delta();
  }

  int64_t get_delay() const
  {
    return summary_item_->delay();
  }

private:
  SummaryItem(int64_t delta=0, int64_t delay=0)
    : delta(delta)
    , delay(delay) {}

  BufferManager::SummaryItem* summary_item_;
};
