#ifndef __COUNTER_H__
#define __COUNTER_H__

#include <ebbrt/Messenger.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/Future.h>
#include <ebbrt/MulticoreEbb.h>
#include <ebbrt/Message.h>
#include <ebbrt/Debug.h>
#include "StaticEbbIds.h"
#include <unordered_map>
#include <ebbrt/Future.h>

namespace Counter {
  class MultinodeCounter : public ebbrt::Messagable<Counter::MultinodeCounter> {
/*
  class Root{
    public:
      Root(ebbrt::EbbId id);
      ebbrt::EbbId myId() { return myId_; }
      CounterRoot * getRep_BIN();
      ebbrt::SharedFuture<std::string> getString() { return data_; }
    private:
      std::mutex lock_;    
      ebbrt::EbbId myId_;
      CounterRoot *theRep_;
      ebbrt::SharedFuture<std::string> data_;
    }; 
*/

  private:
    std::mutex lock_;
    //Root *myRoot_;
    int val_;
    int id_;
    std::vector<ebbrt::Messenger::NetworkId> nodelist;
    std::unordered_map<uint32_t, ebbrt::Promise<int>> promise_map_;
    MultinodeCounter() :  ebbrt::Messagable<Counter::MultinodeCounter>(kCounterEbbId),  val_(0), nodelist{} { ebbrt::kprintf("Constrcting MultinodeCounter \n");}
  public: 
    static MultinodeCounter & HandleFault(ebbrt::EbbId id);
    //static ebbrt::Future<void> Init();
    int Val() { return val_; }
    void Inc() { ebbrt::kprintf("1 BM: inc\n"); val_++; }
    void addTo(ebbrt::Messenger::NetworkId nid){ nodelist.push_back(nid); }
    int size(){ return int(nodelist.size()); }
    ebbrt::Messenger::NetworkId nlist(int i){ return nodelist[i]; }
    void Join(ebbrt::Messenger::NetworkId nid);
    void ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer);
    std::vector<ebbrt::Future<int>> Gather();
  };
//  constexpr auto theCounter = ebbrt::EbbRef<MultinodeCounter>(kCounterEbbId);
};

#endif 
