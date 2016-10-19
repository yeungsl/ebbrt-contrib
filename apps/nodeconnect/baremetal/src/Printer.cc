//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Printer.h"
#include "Connect.h"
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/GlobalIdMap.h>

#include <ebbrt/UniqueIOBuf.h>

EBBRT_PUBLISH_TYPE(, Printer);

Printer::Printer(ebbrt::Messenger::NetworkId nid)
    : Messagable<Printer>(kPrinterEbbId), remote_nid_(std::move(nid)) {}

Printer& Printer::HandleFault(ebbrt::EbbId id) {
  {
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    if (found) {
      auto& pr = *boost::any_cast<Printer*>(accessor->second);
      ebbrt::EbbRef<Printer>::CacheRef(id, pr);
      return pr;
    }
  }

  ebbrt::EventManager::EventContext context;
  auto f = ebbrt::global_id_map->Get(id);
  Printer* p;
  f.Then([&f, &context, &p](ebbrt::Future<std::string> inner) {
    p = new Printer(ebbrt::Messenger::NetworkId(inner.Get()));
    ebbrt::event_manager->ActivateContext(std::move(context));
  });
  ebbrt::event_manager->SaveContext(context);
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<Printer>::CacheRef(id, *p);
    return *p;
  }

  delete p;
  // retry reading
  ebbrt::LocalIdMap::ConstAccessor accessor;
  ebbrt::local_id_map->Find(accessor, id);
  auto& pr = *boost::any_cast<Printer*>(accessor->second);
  ebbrt::EbbRef<Printer>::CacheRef(id, pr);
  return pr;
}

void Printer::Print(const char* str) {
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char*>(buf->MutData()), len, "%s", str);
  SendMessage(remote_nid_, std::move(buf));
}

void Printer::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                             std::unique_ptr<ebbrt::IOBuf>&& buffer) {
  //throw std::runtime_error("Printer: Received message unexpectedly!");
  
  auto output = std::string(reinterpret_cast<const char*>(buffer->Data()),buffer->Length());
  auto p = nid.ToString().c_str();
  char dst[20];
  std::strcpy(dst,  output.c_str());
  auto  numbers = std::strtok(dst,".");
  int ip[4];
  int i = 0;
  while(numbers != NULL){
    ip[i] = std::atoi(numbers);
    numbers = std::strtok(NULL,".");
    i++;
  }
  std::array<uint8_t, 4> ar_ip = {(uint8_t)ip[0],(uint8_t)ip[1],(uint8_t)ip[2],(uint8_t)ip[3]};
  auto rnid = ebbrt::Messenger::NetworkId(std::string(reinterpret_cast<const char*>(ar_ip.data()),4));
  ebbrt::kprintf("recieved something from nid: %s\n", p);
  ebbrt::kprintf("recieved message: %s\n", output.c_str());
  ebbrt::kprintf("recieved message: %s\n", rnid.ToString().c_str());
  worker -> Send(rnid,(const char*)("1."+rnid.ToString()).c_str());
  ebbrt::kprintf("Sent!\n");
}
