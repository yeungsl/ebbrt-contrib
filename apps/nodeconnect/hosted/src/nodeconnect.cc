//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <signal.h>

#include <boost/filesystem.hpp>
#include <iostream>
#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/UniqueIOBuf.h>

#include "Printer.h"
#include "Connect.h"
int main(int argc, char** argv) {
  auto bindir = boost::filesystem::system_complete(argv[0]).parent_path() /
                "/bm/nodeconnect.elf32";

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

      auto ns = ebbrt::node_allocator->AllocateNode(bindir.string());
      ns.NetworkId().Then(
      [&](ebbrt::Future<ebbrt::Messenger::NetworkId> net_if) {
	auto nid = net_if.Get();
	auto nid0 = nid.ToString();
      auto ns_ = ebbrt::node_allocator->AllocateNode(bindir.string());

      ns_.NetworkId().Then(
      [nid0](ebbrt::Future<ebbrt::Messenger::NetworkId> net_if) {
	auto nid = net_if.Get();
	std::cout<<"FRIST IP:"<<nid0<<std::endl;
	printer->Print(nid, nid0.c_str());
      });

      });

    });
  }
  c.Run();

  return 0;
}
