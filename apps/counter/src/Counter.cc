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
  {
    // Trying to find if registered in the local Id map
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    if (found) {
      // If found the cache reference and return the rep pointer
      auto& r = *boost::any_cast<Counter::MultinodeCounter*>(accessor->second);
      ebbrt::EbbRef<Counter::MultinodeCounter>::CacheRef(id, r);
      return r;
    }
  }
  // join with home node, found in global_id_map
#ifdef __ebbrt__
  ebbrt::EventManager::EventContext context;
  auto f = ebbrt::global_id_map->Get(id);
  Counter::MultinodeCounter* p;
  f.Then([&f, &context, &p, id](ebbrt::Future<std::string> inner) {
    p = new Counter::MultinodeCounter();
    p->addTo(ebbrt::Messenger::NetworkId(inner.Get()));
    //potentially dangerous!!! no undo the join, what if race happend here, no lock.
    //the join here is not safe it will not make sure if it joined or not....
    //the design here does not guarentee join upon this node
    //did not fully consider the risk of doing such join
    //    p->Join(ebbrt::Messenger::NetworkId(inner.Get()));
    ebbrt::event_manager->ActivateContext(std::move(context));
  });
  ebbrt::event_manager->SaveContext(context);
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<Counter::MultinodeCounter>::CacheRef(id, *p);
    // This join is eventual join, which do not check if the node is actaully joining the home node
    // however, very efficient
    //    p->Join();
    // Consistent Join is relatively slow but garuentees the node is registered with the home node
    p->ConsistentJoin();
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


void Counter::MultinodeCounter::Join(){
  //has to fix this, right now its just sending a message, which is not enough
  //should maintain the consistency
  ebbrt::kprintf("joining the home node\n");
  if (size() == 1){
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = 0; 
    SendMessage(nlist(0), std::move(buf));
  }
  else{
    ebbrt::kabort("no home node");
  }
}

ebbrt::Future<int> Counter::MultinodeCounter::ConsistentJoin(){
  //this join is a consistent join that hold untill the node itself knows its joined
 ebbrt::kprintf("joining the home node\n");
  if (size() == 1){
    ebbrt::Promise<int> promise;
    auto f = promise.GetFuture();
    int id;
    {
      std::lock_guard<std::mutex> guard(lock_);
      id = 10;
      bool inserted;
      std::tie(std::ignore, inserted) =
	promise_map_.emplace(id, std::move(promise));
      assert(inserted);
    }
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = 0; 
    SendMessage(nlist(0), std::move(buf));
    return f;
  }
  else{
    ebbrt::kabort("no home node");
  }
}

void Counter::MultinodeCounter::ReceiveMessage(ebbrt::Messenger::NetworkId nid, 
				       std::unique_ptr<ebbrt::IOBuf>&& buffer){
  auto dp = buffer->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  ebbrt::kprintf("id is %d\n", id);

  if(id == 0){
    //Join messages recieve simply add to the vector UNSAFE!!!
    //has to add something to make sure no repeating nid
    /*
    printf("0 FE: recieved Join!\n");
    addTo(nid);
    printf("nodelist size=%d\n", size());
    */
    addTo(nid);
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id + 10; 
    SendMessage(nid, std::move(buf));
    return;
  }
  if(id == 10){
    std::lock_guard<std::mutex> guard(lock_);
    auto it = promise_map_.find(id);
    assert(it != promise_map_.end());
    it->second.SetValue(0);
    promise_map_.erase(it);
    return;
  }
  if (id & 1){
    //This way is not generic
    //limit the functionality of each nodes
    // has to set special flags in the switch statement
#ifdef __ebbrt__
    //if its back end, send the local val to home node
    ebbrt::kprintf("RecieveMessage: BM sending local val %d \n", Val());
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id - 1;  // Send back with the original id
    dp.Get<uint32_t>() = Val();
    SendMessage(nid, std::move(buf));
    return;
#else
    //if front end, call gather and reply with the send
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
    //this fufills the promises in the promise map.
    //sort of like committing transactions for each send recieve pair
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
  //generic gather for all the nodes. Then home node will have more than one nids.
  //other nodes will only have one nid which is the home node's nid.
  uint32_t id;
  printf("gather nodelist size=%d\n", size());
  if (size() == 0){
    // if none of the nodes joined, then just do a simple local val
    ebbrt::Promise<int> promise;
    auto f = promise.GetFuture();
    ret.push_back(std::move(f));
    printf("0FE: nothing in the node list yet\n");
    promise.SetValue(0);
    return ret;
  }

  for(int i = 0; i < MultinodeCounter::size(); i++){
    // if some of the nodes come up make a promise of a send n recieve pair
    // fufill the promise when gets back the node value
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

int Counter::MultinodeCounter::GlobalVal(){
  //getting back a vector from gather, simply sum up the counts
  /*
  ebbrt::Promise<int> promise;
  auto f = promise.GetFuture();
  */
  auto vfg = Gather();
  ebbrt::when_all(vfg).Block().Then([this]( auto vf){
      auto v = vf.Get();
      int gather_sum = this->Val();
      for(uint32_t i = 0; i< v.size(); i++){
	gather_sum += v[i];
      }
      //      promise.SetValue(std::move(gather_sum));
      return gather_sum;
    });
  return 0;
}

