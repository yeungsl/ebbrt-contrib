//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <signal.h>

#include <boost/filesystem.hpp>

#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>

#include "Printer.h"

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
      [](ebbrt::Future<ebbrt::Messenger::NetworkId> net_if) {
	auto  net_id = net_if.Get();
	printer->Print(net_id, "something!!");
      });
      auto ns_ = ebbrt::node_allocator->AllocateNode(bindir.string());

      ns_.NetworkId().Then(
      [](ebbrt::Future<ebbrt::Messenger::NetworkId> net_if) {
	auto  net_id = net_if.Get();
	printer->Print(net_id, "something!!");
      });


    });
  }
  c.Run();

  return 0;
}
