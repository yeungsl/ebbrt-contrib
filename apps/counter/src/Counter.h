#ifndef __COUNTER_H__
#define __COUNTER_H__

#include <ebbrt/Messenger.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/Future.h>
#include <ebbrt/Buffer.h>
#include <ebbrt/Message.h>

#include "StaticEbbIds.h"

namespace Counter {
  class SharedCounter {
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
    SharedCounter(Root *root) : myRoot_(root), val_(0) {}
  public:
    static SharedCounter & HandleFault(ebbrt::EbbId id);
    static ebbrt::Future<ebbrt::EbbRef<SharedCounter>> Init();
    int val() { return val_; }
    void inc() { val_++; }
  };

#ifdef __EBBRT_BM__
  __attribute__ ((unused)) static void Init() {}
#else
  __attribute__ ((unused)) static void Init() {
    SharedCounter::Init().Block();
  }
#endif


  constexpr auto theCounter = ebbrt::EbbRef<SharedCounter>(kCounterEbbId);
};

#endif 
