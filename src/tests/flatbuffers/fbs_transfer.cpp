/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#include <iostream>
#include "flatbuffers/fbs_strategy.h"
#include "flatbuffers/FbsManager.h"

using namespace Devcash;

int main(int /*argc*/, const char * /*argv*/ []) {

  DataManager<FlatBufferStrategy> data_manager;
  Devcash::Buffer addr(33);

  auto transfer1 = data_manager.CreateTransfer(addr
                                               , 1
                                               , 2
                                               , 0);

  auto transfer2 = data_manager.CreateTransfer(addr
                                               , 1
                                               , 2
                                               , 2);


  bool is_equal1 = (transfer1 == transfer2);

  transfer2.set_delay(0);

  bool is_equal2 = (transfer1 == transfer2);


  std::cout << "bool1: " << is_equal1 << " bool2: " << is_equal2 << "\n";
}
