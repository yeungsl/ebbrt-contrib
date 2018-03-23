#include <ebbrt/Future.h>
#include <ebbrt/Message.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/Debug.h>
#include "StaticEbbIds.h"
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/Cpu.h>
#include <iostream>
#include <sstream>
#include <ebbrt/SharedEbb.h>
#include "Membership.h"
#ifdef __ebbrt__
#include <ebbrt/native/Cpu.h>
#include <ebbrt/native/VMemAllocator.h>
#include <ebbrt/native/PageAllocator.h>
#include <ebbrt/native/Runtime.h>
#endif

class RemoteMemory : public ebbrt::SharedEbb<RemoteMemory>, public ebbrt::Messagable<RemoteMemory>{
 private:
  std::mutex lock_;
  bool cached = false;
#ifdef __ebbrt__
  uint64_t page_len;

  ebbrt::Pfn pfn_;
  uint64_t page_iteration;
  void sendPage(ebbrt::Messenger::NetworkId dst);
  ebbrt::Messenger::NetworkId FrontendId = ebbrt::Messenger::NetworkId(ebbrt::runtime::Frontend());
#endif

  std::unique_ptr<ebbrt::Promise<int>> promise = std::make_unique<ebbrt::Promise<int>>();

 public:
  RemoteMemory() : ebbrt::Messagable<RemoteMemory>(kRemoteMEbbId) {}
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer);
  bool promise_aval = true;
#ifdef __ebbrt__
  ebbrt::Future<int> QueryMaster();
  ebbrt::Pfn fetchPage();
  void cachePage(uint64_t len, ebbrt::Pfn pfn, uint64_t iteration);
#endif
};
constexpr auto rm = ebbrt::EbbRef<RemoteMemory>(kRemoteMEbbId);
