#include <ebbrt/Future.h>
#include <ebbrt/Message.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/Debug.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/SharedEbb.h>
#include "StaticEbbIds.h"

// Defining command for query master
enum commands {PAGE, OWNERSHIP, FUFILL, SEND, MASTER, FOLLOWER, SUCCESS, CONNECT};

class Membership :  public ebbrt::StaticSharedEbb<Membership>, public ebbrt::Messagable<Membership>{
 private:
  std::mutex lock_;
  std::vector<ebbrt::Messenger::NetworkId> nodelist;
  bool ms_promise = true;
  std::unique_ptr<ebbrt::MutUniqueIOBuf> FufillOwnership(int job);
  void addTo(ebbrt::Messenger::NetworkId nid){
    if (std::find(nodelist.begin(), nodelist.end(), nid) == nodelist.end()){
      ebbrt::kprintf("STORING NID: %s\n",nid.ToString().c_str() );
      nodelist.emplace_back(std::move(nid));
    }
  }
  std::unique_ptr<ebbrt::Promise<int>> promise = std::make_unique<ebbrt::Promise<int>>();

 public:
  Membership() : ebbrt::Messagable<Membership>(kMemberEbbId), nodelist{} {}
  //static void Init();
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer);
  int size(){ return int(nodelist.size()); }
  ebbrt::Messenger::NetworkId Master(){return nodelist[0];}
  ebbrt::Future<int> Query (ebbrt::Messenger::NetworkId nid);
};
constexpr auto ms = ebbrt::EbbRef<Membership>(kMemberEbbId);
