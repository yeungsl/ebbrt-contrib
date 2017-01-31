//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <signal.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <boost/filesystem.hpp>
#include <ebbrt/hosted/Context.h>
#include <ebbrt/hosted/ContextActivation.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/hosted/NodeAllocator.h>
#include <ebbrt/Runtime.h>
#include "../Counter.h"
#include "Printer.h"
#include "../MulticoreCounter.h"

char* ExecName = 0;

void AppMain(){
  auto bindir = boost::filesystem::system_complete(ExecName).parent_path() /
                "/bm/helloworld.elf32";

  auto init_counter = ebbrt::Counter::Create(0);
  init_counter->Up();
  std::cout<<"core get"<< init_counter -> GetLocal() << std::endl;
  std::cout<<"root get"<< init_counter -> GetRoot() << std::endl;
  
  auto ns = ebbrt::node_allocator->AllocateNode(bindir.string(), 4);
  ns.NetworkId().Block();
  
  std::cout<<"list nubmer"<< init_counter -> size() << std::endl;

  auto cpu_1 = ebbrt::Cpu::GetByIndex(1);
  auto context = cpu_1->get_context();
  ebbrt::event_manager->SpawnRemote(([init_counter](){
	    std::chrono::seconds sec(5);
	    std::this_thread::sleep_for(sec);  
	    std::cout<< "slept for 5s" << std::endl;
	    std::cout<<"global get" << init_counter -> GlobalVal() << std::endl;
      }), context);
  

}

int main(int argc, char** argv) {
  void* status;
  
  ExecName = argv[0];

  pthread_t tid = ebbrt::Cpu::EarlyInit(2);
  
  pthread_join(tid, &status);

  ebbrt::Cpu::Exit(0);

  return 0;
}
