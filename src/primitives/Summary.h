/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#include "primitives/SummaryItem.h"

template <typename StorageManager, typename StorageStrategy>
class Summary {
 public:
  /** Constrcutors */
  Summay();
  Summay(std::string canonical);
  Summay(const Summay& other);

  /** Assignment Operators */
  Summay* operator=(Summay&& other)
  {
    if (this != &other) {
      this->summary_ = std::move(other.summary_);
    }
    return this;
  }
  Summay* operator=(Summay& other)
  {
    if (this != &other) {
      this->summary_ = std::move(other.summary_);
    }
    return this;
  }

  /** Adds a summary record to this block.
   *  @param addr the address that this change applies to
   *  @param item a chain state vector summary of transactions.
   *  @return true iff the summary was added
  */
  bool AddItem(const std::string& addr, long coinType, const SummayItem& item);

  /** Adds a summary record to this block.
   *  @param addr the addresses involved in this change
   *  @param coinType the type number for this coin
   *  @param delta change in coins (>0 for receiving, <0 for spending)
   *  @param delay the delay in seconds before this transaction can be received
   *  @return true iff the summary was added
  */
  bool addItem(const std::string& addr, long coinType, long delta, long delay=0);

  /** Adds multiple summary records to this block.
   *  @param addr the addresses involved in these changes
   *  @param items map of Summay items detailing these changes by coinType
   *  @return true iff the summary was added
  */
  bool addItems(std::string addr,
      std::unordered_map<long, SummayItem> items);

  /**
   *  @return a canonical string summarizing these changes.
  */
  std::string toCanonical() const;

  size_t getByteSize() const;

  /**
   *  @return true iff, the summary passes sanity checks
  */
  bool isSane() const;

  typedef boost::container::flat_map<std::string, coinmap> SummaryMap;
  SummaryMap summary_;
  std::mutex lock_;
};
