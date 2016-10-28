#ifndef __COUNTER_H__
#define __COUNTER_H__

#include <ebbrt/Messenger.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/Future.h>
#include <ebbrt/Buffer.h>
#include <ebbrt/Message.h>
#include <ebbrt/Debug.h>
#include "StaticEbbIds.h"
#include <unordered_map>

namespace Counter {
  class SharedCounter : public ebbrt::Messagable<Counter::SharedCounter>{
    class Root {
    private:
      std::mutex lock_;
      ebbrt::EbbId myId_;
      SharedCounter *theRep_;
      ebbrt::SharedFuture<std::string> data_;
    public:
      Root(ebbrt::EbbId id);
      ebbrt::EbbId myId() { return myId_; }
      SharedCounter * getRep_BIN();
      ebbrt::SharedFuture<std::string> getString() { return data_; }
    }; 
  private:
    Root *myRoot_;
    int val_;
//std::mutex m_;
    std::vector<ebbrt::Messenger::NetworkId> nodelist;
//  std::unordered_map<uint32_t, ebbrt::Promise<int>> promise_map_;
    SharedCounter(Root *root) :  ebbrt::Messagable<Counter::SharedCounter>(kCounterEbbId), myRoot_(root), val_(0), nodelist{} {}
  public:
    static SharedCounter & HandleFault(ebbrt::EbbId id);
    static ebbrt::Future<ebbrt::EbbRef<SharedCounter>> Init();
    int val() { return val_; }
    void inc() { val_++; }
    void addTo(ebbrt::Messenger::NetworkId nid){ nodelist.push_back(nid); }
    int size(){ return int(nodelist.size()); }
    ebbrt::Messenger::NetworkId nlist(int i){ return nodelist[i]; }
    void join();
    void ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer);
    ebbrt::Future<int> gather();
  };

#ifdef __ebbrt__
  __attribute__ ((unused)) static void Init() {
    ebbrt::kprintf("1 BM: counter initiated\n");
  }
#else
  __attribute__ ((unused)) static void Init() {
    SharedCounter::Init().Block();
    ebbrt::kprintf("header, id %s\n", ebbrt::messenger->LocalNetworkId().ToString().c_str());    
  }
#endif
  constexpr auto theCounter = ebbrt::EbbRef<SharedCounter>(kCounterEbbId);
};

#endif 
