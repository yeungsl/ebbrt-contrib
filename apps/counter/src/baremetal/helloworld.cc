//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Printer.h"
#include "../Counter.h"
#include "../MulticoreCounter.h"

void AppMain() {
  
  ebbrt::kprintf("1 BM: back end up!\n");
  //initialize the multinode counter
  // Counter::node_counter->Inc();
  //initialize the multicorenode counter
  ebbrt::EbbRef<ebbrt::Counter> counter(ebbrt::ebb_allocator->Allocate());
  ebbrt::kprintf("1 BM: Inc counter\n");
  /*
  //Using future Gather() faster
  auto f = Counter::node_counter->Gather();
  when_all(f).Then([&](auto vf){
    ebbrt::kprintf("1 BM: gather sum %d\n", vf.Get()[0]);
  });
  //Using int gather() slow; thread has to be blocked

  auto r = Counter::node_counter->GlobalVal();
  ebbrt::kprintf("sum: %d\n", r);
  */
}
