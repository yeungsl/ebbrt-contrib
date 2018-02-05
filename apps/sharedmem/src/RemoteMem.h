#include <ebbrt/Future.h>
#include "StaticEbbIds.h"
#include <ebbrt/Message.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/Debug.h>
#include "StaticEbbIds.h"
#include <unordered_map>
#include <boost/container/static_vector.hpp>
#include <ebbrt/native/PMem.h>
// Defining command for query master
enum commands {PAGE, OWNERSHIP, FUFILL, SEND, MASTER, FOLLOWER, SUCCESS, CONNECT};

class RemoteMemory : public ebbrt::Messagable<RemoteMemory>{
 private:
  std::mutex lock_;
  std::vector<ebbrt::Messenger::NetworkId> nodelist;
  bool cached = false;
  ebbrt::ExplicitlyConstructed<boost::container::static_vector<uint32_t, (int)ebbrt::pmem::kPageSize/4>> tem_buffer;
  std::unordered_map<uint32_t, ebbrt::Promise<int>> promise_map_;
  RemoteMemory() : ebbrt::Messagable<RemoteMemory>(kRemoteMEbbId), nodelist{} {}
  void addTo(ebbrt::Messenger::NetworkId nid){
    if (std::find(nodelist.begin(), nodelist.end(), nid) == nodelist.end()){
      nodelist.push_back(nid);
    }
  }
  void sendPage(ebbrt::Messenger::NetworkId dst);
 public:
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer);
  static RemoteMemory & HandleFault(ebbrt::EbbId id);
  ebbrt::Future<int> QueryMaster(int command);
  int size(){ return int(nodelist.size()); }
  void fetchPage(volatile uint32_t* pptr);
  void cachePage(uint64_t len, volatile uint32_t * pptr, uint64_t iteration);
};
constexpr auto rm = ebbrt::EbbRef<RemoteMemory>(kRemoteMEbbId);
