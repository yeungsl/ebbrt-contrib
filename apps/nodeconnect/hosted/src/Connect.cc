//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Connect.h"

#include <iostream>

#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/UniqueIOBuf.h>

EBBRT_PUBLISH_TYPE(, Worker);

Worker::Worker() : ebbrt::Messagable<Worker>(kWorkerEbbId) {}

ebbrt::Future<void> Worker::Init() {
  return ebbrt::global_id_map->Set(
      kWorkerEbbId, ebbrt::messenger->LocalNetworkId().ToBytes());
}

void Worker::Send(ebbrt::Messenger::NetworkId remote_nid_, const char* str) {
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char*>(buf->MutData()), len, "%s", str);
  SendMessage(remote_nid_, std::move(buf));
}

void Worker::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                             std::unique_ptr<ebbrt::IOBuf>&& buffer) {
  auto output = std::string(reinterpret_cast<const char*>(buffer->Data()),
                            buffer->Length());
  std::cout << nid.ToString() << ": " << output;
}
