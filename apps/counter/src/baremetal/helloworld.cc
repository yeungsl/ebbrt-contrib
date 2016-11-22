//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Printer.h"
#include "../Counter.h"

auto node_counter = ebbrt::EbbRef<Counter::MultinodeCounter>(kCounterEbbId);

void AppMain() {
  
  ebbrt::kprintf("1 BM: back end up!\n");

  //ebbrt::MulticoreEbbRoot<Counter::MultinodeCounter> mncount;
  //ebbrt::EbbRef<Counter> counter = Counter::Create(&counter_root);
  node_counter->Inc();
  //Counter::theCounter->inc();
  ebbrt::kprintf("1 BM: back end counter\n");
  //  Counter::theCounter->join();
  //  ebbrt::kprintf("finished\n");
}
