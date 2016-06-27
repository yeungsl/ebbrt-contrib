//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef APPS_NODECONNECT_BAREMETAL_SRC_PRINTER_H_
#define APPS_NODECONNECT_BAREMETAL_SRC_PRINTER_H_

#include <string>

#include <ebbrt/Message.h>

#include "../../src/StaticEbbIds.h"

class Worker : public ebbrt::Messagable<Worker> {
 public:
  explicit Worker(ebbrt::Messenger::NetworkId nid);

  static Worker& HandleFault(ebbrt::EbbId id);

  void Send(const char* string);
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                      std::unique_ptr<ebbrt::IOBuf>&& buffer);

 private:
  ebbrt::Messenger::NetworkId remote_nid_;
};

constexpr auto worker = ebbrt::EbbRef<Worker>(kWorkerEbbId);

#endif  // APPS_NODECONNECT_BAREMETAL_SRC_PRINTER_H_
