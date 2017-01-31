#ifndef __COUNTER_H__
#define __COUNTER_H__


#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/Future.h>
#include <ebbrt/Message.h>
#include <ebbrt/Debug.h>
#include "StaticEbbIds.h"
#include <unordered_map>
#include <ebbrt/Future.h>

namespace MNCounter {
  class MultinodeCounter : public ebbrt::Messagable<MNCounter::MultinodeCounter> {
  private:
    std::mutex lock_;
    int val_;
    int id_;
    std::vector<ebbrt::Messenger::NetworkId> nodelist;
    std::unordered_map<uint32_t, ebbrt::Promise<int>> promise_map_;
    void addTo(ebbrt::Messenger::NetworkId nid){
      if (std::find(nodelist.begin(), nodelist.end(), nid) == nodelist.end()){
	nodelist.push_back(nid);
      }
    }
    int size(){ return int(nodelist.size()); }
    void Join();
    ebbrt::Future<int> ConsistentJoin();
    ebbrt::Messenger::NetworkId nlist(int i){ return nodelist[i]; }
    MultinodeCounter() :  ebbrt::Messagable<MNCounter::MultinodeCounter>(kCounterEbbId),  val_(0), nodelist{} { ebbrt::kprintf("Constrcting MultinodeCounter \n");}
    std::vector<ebbrt::Future<int>> Gather();
  public: 
    static MultinodeCounter & HandleFault(ebbrt::EbbId id);
    int Val() { return val_; }
    void Inc() { ebbrt::kprintf("1 BM: inc\n"); val_++; }
    void ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer);
    int GlobalVal();
  };
  constexpr auto node_counter = ebbrt::EbbRef<MultinodeCounter>(kCounterEbbId);

};

#endif 
