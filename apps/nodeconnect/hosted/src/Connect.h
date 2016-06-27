//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef APPS_NODECONNECT_HOSTED_SRC_CONNECT_H_
#define APPS_NODECONNECT_HOSTED_SRC_CONNECT_H_

#include <string>

#include <ebbrt/Message.h>
#include <ebbrt/StaticSharedEbb.h>

#include "../../src/StaticEbbIds.h"

class Worker : public ebbrt::StaticSharedEbb<Worker>,
                public ebbrt::Messagable<Worker> {
 public:
  Worker();

  static ebbrt::Future<void> Init();
  void Send(ebbrt::Messenger::NetworkId remote_nid_, const char* string);
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                      std::unique_ptr<ebbrt::IOBuf>&& buffer);
};

constexpr auto worker = ebbrt::EbbRef<Worker>(kWorkerEbbId);

#endif  // APPS_NODECONNECT_HOSTED_SRC_CONNECT_H_
