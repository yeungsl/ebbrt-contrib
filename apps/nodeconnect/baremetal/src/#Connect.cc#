//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Connect.h"
#include <string>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/UniqueIOBuf.h>

EBBRT_PUBLISH_TYPE(, Worker);

std::array<uint8_t, 4> ip = {0,0,0,0};

auto self = ebbrt::Messenger::NetworkId(std::string(reinterpret_cast<const char*>(ip.data()),4));

auto partner = ebbrt::Messenger::NetworkId(std::string(reinterpret_cast<const char*>(ip.data()),4));

bool ready;

int sharedSize;

bool owner;

uint32_t * sharedAddr;


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

void Worker::Claim(uint32_t* addr, int i){
  if (!ready){
    throw std::runtime_error("Worker: Do not have a partner yet \n");
  }
  char ss[20];
  std::sprintf(ss, "%p.%d",addr, ((int)ebbrt::pmem::kPageSize*(1<<i))/4);
  auto saddr = std::string(ss);
  Send(partner, (const char*)("3." +saddr).c_str());
  ebbrt::kprintf("Claim: sending %s\n", saddr.c_str());
}

int Worker::Qury(uint32_t* addr){
  if (sharedAddr != 0 || addr < sharedAddr + sharedSize || addr > sharedAddr || owner == false){
    char ss[20];
    std::sprintf(ss, "%p",addr);
    auto saddr = std::string(ss);
    Send(partner, (const char*)("4." +saddr).c_str());
  }
  int v = *addr;
  return  v;
}


void Worker::Send(ebbrt::Messenger::NetworkId nid, const char* str) {
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char*>(buf->MutData()), len, "%s", str);
  SendMessage(nid, std::move(buf));
}

void Worker::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                             std::unique_ptr<ebbrt::IOBuf>&& buffer) {
  auto output = std::string(reinterpret_cast<const char*>(buffer->Data()),buffer->Length());
  partner = nid;
  ebbrt::kprintf("recieved message: %s from: %s\n", output.c_str(),nid.ToString().c_str());
  char dst[20];
  std::strcpy(dst,  output.c_str());
  auto inst = std::strtok(dst, ".");
  auto command = std::atoi(inst);
  if(command == 1 || command == 2){
    auto inst = std::strtok(NULL, ".");
    int ip[4];
    int i = 0;
    while(inst != NULL){
      ip[i] = std::atoi(inst);
      inst = std::strtok(NULL,".");
      i++;
    }
    std::array<uint8_t, 4> ar_ip = {(uint8_t)ip[0],(uint8_t)ip[1],(uint8_t)ip[2],(uint8_t)ip[3]};
    auto lnid = ebbrt::Messenger::NetworkId(std::string(reinterpret_cast<const char*>(ar_ip.data()),4));
    self = lnid;
    if (command == 1){
      Send(partner, (const char*)("2."+ partner.ToString()).c_str());
    }
    ebbrt::kprintf("two nodes connected! self ip: %s, partner ip: %s\n", self.ToString().c_str(), partner.ToString().c_str());
    ready = true;
    if (command == 2){
      auto pfn = ebbrt::page_allocator->Alloc(1);
      auto pptr = (uint32_t *)pfn.ToAddr();
      ebbrt::kprintf("page allocated address: 0x%lx\n",(uintptr_t)pptr);
      Claim(pptr, 1);
      owner = true;
      sharedAddr = pptr;
      sharedSize = 2048;
    }
  }
  if (command == 3){
    auto inst = std::strtok(NULL,".");
    ebbrt::kprintf("inst: %s\n", inst);
    auto addr = std::strtol(inst, NULL, 0);
    ebbrt::kprintf("address: %lx\n",addr);
    sharedAddr = (uint32_t *) addr;
    inst = std::strtok(NULL,".");
    sharedSize = std::atoi(inst);
    ebbrt::kprintf("size: %d\n", sharedSize);
    owner = false;
  }
  if (command  == 4){
    auto inst = std::strtok(NULL, ".");
    auto addr = std::strtol(inst, NULL, 0);
    if (owner == true){
    char ss[20];
    std::sprintf(ss, "%ld", *addr);
    auto saddr = std::string(ss);
    Send(partner, (const char*)("5." +saddr).c_str());
    }
  }
  // if (command == 5){
    
  // }
}
