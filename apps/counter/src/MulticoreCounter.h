//          Copyright Boston University SESA Group 2013 - 2016.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef EBBRT_ZKCOUNTER_H_
#define EBBRT_ZKCOUNTER_H_

#include "MulticoreEbb.h"
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/LocalIdMap.h>
#include "StaticEbbIds.h"
#include <ebbrt/Messenger.h>
#include <ebbrt/Message.h>
#include <string>

namespace ebbrt {
class Counter;

class CounterRoot : public ebbrt::MulticoreEbbRoot<CounterRoot, Counter>{
   using MulticoreEbbRoot<CounterRoot, Counter>::MulticoreEbbRoot;
private:
  std::mutex lock_;
  std::vector<ebbrt::Messenger::NetworkId> nodelist;
  int init_ = 0;
public:
 CounterRoot(int init, ebbrt::EbbId id)
   : MulticoreEbbRoot<CounterRoot, Counter>(id),  nodelist{}, init_(init) {}
  uint64_t Get();
  void addTo(ebbrt::Messenger::NetworkId nid){
    {
      std::lock_guard<std::mutex> guard(lock_);
      if (std::find(nodelist.begin(), nodelist.end(), nid) == nodelist.end()){
	nodelist.push_back(nid);
      }
    }
    return;
  }
  ebbrt::Messenger::NetworkId nlist(int i){ return nodelist[i]; }
  int size(){ return int(nodelist.size()); }
  std::unordered_map<uint32_t, ebbrt::Promise<int>> promise_map_;
};

class Counter : public ebbrt::MulticoreEbb<Counter, CounterRoot>, public ebbrt::Messagable<Counter>{
  using MulticoreEbb<Counter, CounterRoot>::MulticoreEbb;
private:
  std::mutex lock_;
  ebbrt::Future<int> ConsistentJoin( ebbrt::CounterRoot * root );
  std::vector<ebbrt::Future<int>> Gather();
  ebbrt::Messenger::NetworkId nlist(int i){ return root_->nlist(i); };
  void addTo(ebbrt::Messenger::NetworkId nid){  root_ -> addTo(nid);  };
  int decode(int id);
  void nodeReply(ebbrt::Messenger::NetworkId nid,  uint32_t id, int val);
  void homeReply(ebbrt::Messenger::NetworkId nid, uint32_t id, int val);
  void JoinResponse(ebbrt::Messenger::NetworkId nid);
  void Fullfill(int val, uint32_t id);
  uint64_t count_ = 0;
public:
  // implicit use of template constructors
  Counter(ebbrt::EbbId id)
    : MulticoreEbb<Counter, CounterRoot>(id), ebbrt::Messagable<Counter>(id){}
  static ebbrt::EbbRef<Counter> Create(int init, ebbrt::EbbId id = kNcCounterEbbId);
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer);  
  void Up();
  void Down();
  uint64_t GetLocal();
  uint64_t GetRoot();
  int size(){ return root_->size(); }
  int GlobalVal();
};
};     // end namespace
#endif // EBBRT_ZKGLOBALIDMAP_H_
