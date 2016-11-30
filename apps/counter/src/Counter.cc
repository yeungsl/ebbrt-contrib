#include <ebbrt/Runtime.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/Debug.h>
#include <ebbrt/Future.h>
#include "Counter.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <ebbrt/MulticoreEbb.h>


using namespace Counter;
EBBRT_PUBLISH_TYPE(, MultinodeCounter);


#if 0
ebbrt::Future<void>
Counter::MultinodeCounter::Init()
{

  return ebbrt::global_id_map->Set(kCounterEbbId, ebbrt::messenger->LocalNetworkId().ToBytes())
      .Then([](ebbrt::Future<void> f) {
        f.Get();
        return ;

      });


}
#endif

#if 0
Counter::MultinodeCounter::Root::Root(ebbrt::EbbId id) : myId_(id), theRep_(NULL)  {
  data_ = ebbrt::global_id_map->Get(id).Share();
}

Counter::MultinodeCounter *
Counter::MultinodeCounter::Root::getRep_BIN() {
  std::lock_guard<std::mutex> lock{lock_};
  if (theRep_) return theRep_; // implicity drop lock

  if (theRep_ == NULL)  { 
    // now that we are ready create the rep if necessary
    theRep_ = new MultinodeCounter(this);
  }
  return theRep_;      // implicity drop lock
}
#endif

Counter::MultinodeCounter & 
Counter::MultinodeCounter::HandleFault(ebbrt::EbbId id) {
  // Check if we have a rep stored in the local_id_map:
  ebbrt::kprintf("handle fault at all (%d)?\n", id);
  {
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    if (found) {
      ebbrt::kprintf("found?\n");
      auto& r = *boost::any_cast<Counter::MultinodeCounter*>(accessor->second);
      ebbrt::EbbRef<Counter::MultinodeCounter>::CacheRef(id, r);
      return r;
    }
  }
  // join with home node, found in global_id_map
#ifdef __ebbrt__
  ebbrt::EventManager::EventContext context;
  auto f = ebbrt::global_id_map->Get(id);
  ebbrt::kprintf("1BM: get from global id map?\n");
  Counter::MultinodeCounter* p;
  f.Then([&f, &context, &p, id](ebbrt::Future<std::string> inner) {
    p = new Counter::MultinodeCounter();
    ebbrt::kprintf("1BM: did I called join?\n");
    p->Join(ebbrt::Messenger::NetworkId(inner.Get()));
    ebbrt::event_manager->ActivateContext(std::move(context));
  });
  ebbrt::event_manager->SaveContext(context);
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<Counter::MultinodeCounter>::CacheRef(id, *p);
    return *p;
  }
#else
  Counter::MultinodeCounter* p;
    ebbrt::global_id_map->Set(id, ebbrt::messenger->LocalNetworkId().ToBytes());
    p = new Counter::MultinodeCounter();
    ebbrt::EbbRef<Counter::MultinodeCounter>::CacheRef(id, *p);
    auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
    if (inserted) {
    ebbrt::EbbRef<Counter::MultinodeCounter>::CacheRef(id, *p);
    return *p;
  }
#endif
  delete p;

  // retry reading
  ebbrt::LocalIdMap::ConstAccessor accessor;
  ebbrt::local_id_map->Find(accessor, id);
  auto& r = *boost::any_cast<Counter::MultinodeCounter*>(accessor->second);
  ebbrt::EbbRef<Counter::MultinodeCounter>::CacheRef(id, r);
  return r;
}


void Counter::MultinodeCounter::Join(ebbrt::Messenger::NetworkId nid){
  ebbrt::kprintf("1BM : got nid: %s\n", nid.ToString().c_str());
  ebbrt::kprintf("joining the home node\n");
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = 0; 
  SendMessage(nid, std::move(buf));
}

void Counter::MultinodeCounter::ReceiveMessage(ebbrt::Messenger::NetworkId nid, 
				       std::unique_ptr<ebbrt::IOBuf>&& buffer){
  auto dp = buffer->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  ebbrt::kprintf("id is %d\n", id);

  if(id == 0){
    printf("0 FE: recieved Join!\n");
    addTo(nid);
    printf("nodelist size=%d\n", size());
    return;
  }
  if (id & 1){
#ifdef __ebbrt__
    ebbrt::kprintf("RecieveMessage: BM sending local val %d \n", Val());
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id - 1;  // Send back with the original id
    dp.Get<uint32_t>() = Val();
    SendMessage(nid, std::move(buf));
    return;
#else
    printf("ReceiveMessage: FE calling Gather!\n");
    auto vfg = Gather();
    auto v = ebbrt::when_all(vfg).Then([this, nid, id](auto vf){
        auto v = vf.Get();
        int gather_sum = this->Val();
        for(uint32_t i = 0; i< v.size(); i++){
  	  gather_sum += v[i];
       }
	ebbrt::kprintf("sum: %d\n", gather_sum);
	auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
	auto dp = buf->GetMutDataPointer();
	dp.Get<uint32_t>() = id - 1;
	dp.Get<uint32_t>() = gather_sum;
	this->SendMessage(nid, std::move(buf));
	});
    return;
#endif
  }
  if (!(id & 1)){
    auto val = dp.Get<uint32_t>();
    ebbrt::kprintf("RecieveMessage: recieved remote val %d\n", val);
    std::lock_guard<std::mutex> guard(lock_);
    auto it = promise_map_.find(id);
    assert(it != promise_map_.end());
    it->second.SetValue(val);
    promise_map_.erase(it);
    return;
  }
}


std::vector<ebbrt::Future<int>> Counter::MultinodeCounter::Gather(){
  std::vector<ebbrt::Future<int>> ret;
#ifdef __ebbrt__
  ebbrt::kprintf("1BM : start gather\n");
  auto f = ebbrt::global_id_map->Get(kCounterEbbId);
  auto ff = f.Block();
  auto nid = ebbrt::Messenger::NetworkId(ff.Get());
  ebbrt::kprintf("1BM : got nid: %s\n", nid.ToString().c_str());
  addTo(nid);
  /*
  ebbrt::Promise<int> promise;
  auto p_f = promise.GetFuture();
  ret.push_back(std::move(p_f));
  auto id = 2;
  {
    std::lock_guard<std::mutex> guard(lock_);
    bool inserted;
    std::tie(std::ignore, inserted) =
      promise_map_.emplace(id, std::move(promise));
    assert(inserted);
  }
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = id + 1; 
  SendMessage(nid, std::move(buf));
  return ret;
  */
#endif
  uint32_t id;
  printf("gather nodelist size=%d\n", size());
  if (size() == 0){
    ebbrt::Promise<int> promise;
    auto f = promise.GetFuture();
    ret.push_back(std::move(f));
    printf("0FE: nothing in the node list yet\n");
    promise.SetValue(0);
    return ret;
  }

  for(int i = 0; i < MultinodeCounter::size(); i++){
    auto nid = MultinodeCounter::nlist(i);
    ebbrt::Promise<int> promise;
    auto f = promise.GetFuture();
    ret.push_back(std::move(f));
    {
      std::lock_guard<std::mutex> guard(lock_);
      id = 2 * (i+1);
      bool inserted;
      // insert our promise into the hash table
      std::tie(std::ignore, inserted) =
	promise_map_.emplace(id, std::move(promise));
      assert(inserted);
    }
    // Construct and send the ping message
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id + 1;  // Ping messages are odd
    SendMessage(nid, std::move(buf));
  }
  return ret;
}

