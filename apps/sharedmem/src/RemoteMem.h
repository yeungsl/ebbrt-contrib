#include <ebbrt/Future.h>
#include "StaticEbbIds.h"
#include <ebbrt/Message.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/Debug.h>
#include "StaticEbbIds.h"
#include <unordered_map>


class RemoteMemory : public ebbrt::Messagable<RemoteMemory>{
 private:
  std::mutex lock_;
  std::vector<ebbrt::Messenger::NetworkId> nodelist;
  std::vector<uint64_t> tem_buffer;
  std::unordered_map<uint32_t, ebbrt::Promise<int>> promise_map_;
  RemoteMemory() : ebbrt::Messagable<RemoteMemory>(kRemoteMEbbId), nodelist{} {}
  void addTo(ebbrt::Messenger::NetworkId nid){
    if (std::find(nodelist.begin(), nodelist.end(), nid) == nodelist.end()){
      nodelist.push_back(nid);
    }
  }
  ebbrt::Messenger::NetworkId nlist(int i){ return nodelist[i]; }
 public:
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer);
  static RemoteMemory & HandleFault(ebbrt::EbbId id);
  int size(){ return int(nodelist.size()); }  
  ebbrt::Future<int> ConsistentJoin();
  void sendPage(uint64_t len, volatile uint32_t * pptr, uint64_t iteration);
  ebbrt::Future<int> queryPage();
  void getPage(volatile uint32_t* pptr);
};
