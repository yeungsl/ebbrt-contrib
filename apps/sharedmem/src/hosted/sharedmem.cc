//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <signal.h>
#include <thread>
#include <boost/filesystem.hpp>
#include <chrono>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/hosted/Context.h>
#include <ebbrt/hosted/ContextActivation.h>
#include <ebbrt/hosted/NodeAllocator.h>
#include "../RemoteMem.h"
#include "Printer.h"
#include <iostream>
int main(int argc, char** argv) {
  auto bindir = boost::filesystem::system_complete(argv[0]).parent_path() /
                "/bm/sharedmem.elf32";

  ebbrt::Runtime runtime;
  ebbrt::Context c(runtime);
  boost::asio::signal_set sig(c.io_service_, SIGINT);
  {
    ebbrt::ContextActivation activation(c);

    // ensure clean quit on ctrl-c
    sig.async_wait([&c](const boost::system::error_code& ec,
                        int signal_number) { c.io_service_.stop(); });
    Printer::Init().Then([bindir](ebbrt::Future<void> f) {
  f.Get();
	ebbrt::NodeAllocator::NodeArgs args = {};
	args.cpus = (uint8_t) 2;

  //rm->Init();
  ebbrt::kprintf("CREATEING THE RM OBJECT WITH EBBID %d\n", kRemoteMEbbId);
  auto instance = new RemoteMemory;
  instance->Create(instance, kRemoteMEbbId);
  new RemoteMemory;
  ebbrt::node_allocator->AllocateNode(bindir.string(), args);
  std::chrono::seconds sec(20);
  std::this_thread::sleep_for(sec);
  ebbrt::node_allocator->AllocateNode(bindir.string(), args);

      });
  }
  c.Run();

  return 0;
}
