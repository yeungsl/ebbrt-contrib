//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Connect.h"

#include <ebbrt/LocalIdMap.h>
#include <ebbrt/GlobalIdMap.h>

#include <ebbrt/UniqueIOBuf.h>

EBBRT_PUBLISH_TYPE(, Worker);

Worker::Worker()
    : Messagable<Worker>(kWorkerEbbId) {}

Worker& Worker::HandleFault(ebbrt::EbbId id) {
  {
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    if (found) {
      auto& pr = *boost::any_cast<Worker*>(accessor->second);
      ebbrt::EbbRef<Worker>::CacheRef(id, pr);
      return pr;
    }
  }

  ebbrt::EventManager::EventContext context;
  Worker* p;

    p = new Worker();

  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<Worker>::CacheRef(id, *p);
    return *p;
  }

  delete p;
  // retry reading
  ebbrt::LocalIdMap::ConstAccessor accessor;
  ebbrt::local_id_map->Find(accessor, id);
  auto& pr = *boost::any_cast<Worker*>(accessor->second);
  ebbrt::EbbRef<Worker>::CacheRef(id, pr);
  return pr;
}

void Worker::Send(ebbrt::Messenger::NetworkId nid, const char* str) {
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char*>(buf->MutData()), len, "%s", str);
  SendMessage(nid, std::move(buf));
}

void Worker::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                             std::unique_ptr<ebbrt::IOBuf>&& buffer) {
  //    throw std::runtime_error("Printer: Received message unexpectedly!");
  ebbrt::kprintf("recieved message from: %s\n", nid.ToString().c_str());
}
