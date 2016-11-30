//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Printer.h"
#include "../Counter.h"

auto node_counter = ebbrt::EbbRef<Counter::MultinodeCounter>(kCounterEbbId);

void AppMain() {
  
  ebbrt::kprintf("1 BM: back end up!\n");
  node_counter->Inc();
  ebbrt::kprintf("1 BM: Inc counter\n");

  auto f = node_counter->Gather();
  when_all(f).Then([&](auto vf){
    ebbrt::kprintf("1 BM: gather sum %d\n", vf.Get()[0]);
  });

}
