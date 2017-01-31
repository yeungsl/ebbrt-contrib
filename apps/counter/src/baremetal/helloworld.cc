//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Printer.h"
#include "../Counter.h"
#include "../MulticoreCounter.h"
#include <ebbrt/SpinBarrier.h>

void AppMain() {
  
  ebbrt::kprintf("1 BM: back end up!\n");

  auto counter = ebbrt::Counter::Create(0);
  auto barrier = new ebbrt::SpinBarrier(ebbrt::Cpu::Count());
  for (size_t core = 1; core < ebbrt::Cpu::Count(); ++core) {
    ebbrt::event_manager->SpawnRemote(
	   [counter, &barrier]() {
            counter->Up();
            barrier->Wait();
            },
          core);
    }
    counter->Up();
    barrier->Wait();
    ebbrt::kprintf("1 BM: Inc counter\n");
    if((size_t)ebbrt::Cpu::GetMine() == 0){
	ebbrt::kprintf("1 BM: root count %d\n", (int)counter->GetRoot());
    }
    ebbrt::kprintf("1 BM: global val %d\n", counter->GlobalVal());

}
