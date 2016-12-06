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



ebbrt::Runtime runtime;

void gatherloop(){
  ebbrt::Context c_2(runtime);
  ebbrt::ContextActivation activation(c_2);
  auto node_counter = ebbrt::EbbRef<Counter::MultinodeCounter>(kCounterEbbId);
  while(true){

    auto sum = node_counter->GlobalVal();
    std::cout<<"global sum: "<< sum <<std::endl;
    /*
    auto vfg = node_counter->Gather();
    when_all(vfg).Then([&](auto vf){
        auto v = vf.Get();
        int gather_sum = node_counter->Val();
        for(uint32_t i = 0; i< v.size(); i++){
  	  gather_sum += v[i];
          std::cout<<"success gather "<< v[i] <<" on node #"<< i <<" gather sum "<< gather_sum <<"\n"<<std::endl;
       }
      });
    */
    std::chrono::seconds sec(3);
    std::this_thread::sleep_for(sec);
  }
}

void nodeloop(char** argv){
  auto bindir = boost::filesystem::system_complete(argv[0]).parent_path() /
                "/bm/helloworld.elf32";

  ebbrt::Context c_3(runtime);
  ebbrt::ContextActivation activation(c_3);
  for(int i = 0; i < 2; i++){
    ebbrt::node_allocator->AllocateNode(bindir.string(), 1);
    std::chrono::seconds sec(5);
    std::this_thread::sleep_for(sec);
  }
}

int main(int argc, char** argv) {
  auto bindir = boost::filesystem::system_complete(argv[0]).parent_path() /
                "/bm/helloworld.elf32";



  ebbrt::Context c(runtime);
  boost::asio::signal_set sig(c.io_service_, SIGINT);
  {
    ebbrt::ContextActivation activation(c);
    // ensure clean quit on ctrl-c
    sig.async_wait([&c](const boost::system::error_code& ec,
                        int signal_number) { c.io_service_.stop(); });
    Counter::node_counter->Inc();
    auto ns = ebbrt::node_allocator->AllocateNode(bindir.string(), 2);
    ns.NetworkId().Then([&](ebbrt::Future<ebbrt::Messenger::NetworkId> net_if){
	    /*
	    std::thread t(gatherloop);
	    t.detach();
	    std::thread t_2(nodeloop, argv);
	    t_2.detach();
	    */
	auto r = Counter::node_counter->GlobalVal();
	std::cout<< "gather_sum "<< r << std::endl;
      });
	
  }
  c.Run();

  return 0;
}
