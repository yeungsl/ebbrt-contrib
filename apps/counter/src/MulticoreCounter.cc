//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/Debug.h>
#include <ebbrt/GlobalIdMap.h>
#include "MulticoreCounter.h"
#include <cstdlib>
#include <string>

using namespace ebbrt;
EBBRT_PUBLISH_TYPE(, Counter);

void ebbrt::Counter::Up() { ++count_; };
void ebbrt::Counter::Down() { --count_; };
uint64_t ebbrt::Counter::GetLocal() { return count_; };
uint64_t ebbrt::Counter::GetRoot() {   return root_->Get(); };

uint64_t ebbrt::CounterRoot::Get() {
  auto sum = init_;
  for (size_t core = 0; core < ebbrt::Cpu::Count(); ++core) {
    auto it = reps_.find(core);
    if (it != reps_.end()) {
      sum += it->second->GetLocal();
    } else {
      ebbrt::kprintf("skipped core %d!\n", core);
    }
  }
  return sum;
};

ebbrt::EbbRef<Counter>
ebbrt::Counter::Create(int init, ebbrt::EbbId id){
    LocalIdMap::Accessor accessor;
    auto created = local_id_map->Insert(accessor, id);
    if(!created){
      ebbrt::kabort();
    }
    auto root = new CounterRoot(init, id);
    accessor->second = root;
    // Root created, but Counter rep not created yet
    // so making the home node to insert network id into global id map
    // and back end could also grab the network id from the global id map
    // assuming everyting the home node get allocated first
#ifdef __ebbrt__
    //trying to get the network id of home node
    ebbrt::EventManager::EventContext context;
    auto f = ebbrt::global_id_map->Get(id);
    ebbrt::Counter* p;
    ebbrt::kprintf("is this whats blocking?\n");
    f.Then([&f, &p, &root, &context, id](ebbrt::Future<std::string> inner) {
	ebbrt::kprintf("adding node id\n");
	auto nid = ebbrt::Messenger::NetworkId(inner.Get());
	root->addTo(nid);
	ebbrt::event_manager->ActivateContext(std::move(context));    
      });
    ebbrt::event_manager->SaveContext(context);
    p = new ebbrt::Counter(id);
    auto f_c = p->ConsistentJoin(root);
    f_c.Then([&p](ebbrt::Future<int> inner){
	assert(inner.Get() == 0);
      });
#else
    //setting the network id of home node
    ebbrt::kprintf("home node id is %s\n", ebbrt::messenger->LocalNetworkId().ToString().c_str());
    ebbrt::global_id_map->Set(id, ebbrt::messenger->LocalNetworkId().ToBytes());
#endif
    return ebbrt::EbbRef<Counter>(id);
};

void ebbrt::Counter::ReceiveMessage(ebbrt::Messenger::NetworkId nid, 
				       std::unique_ptr<ebbrt::IOBuf>&& buffer){
  auto dp = buffer->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  ebbrt::kprintf("id is %d\n", id);

  if(id == 0){
    addTo(nid);
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id + 10; 
    SendMessage(nid, std::move(buf));
    return;
  }
  if(id == 10){
    std::lock_guard<std::mutex> guard(lock_);
    auto it = root_->promise_map_.find(id);
    assert(it != root_->promise_map_.end());
    it->second.SetValue(0);
    root_->promise_map_.erase(it);
    return;
  }

  if (id & 1){
    //This way is not generic
    //limit the functionality of each nodes
    // has to set special flags in the switch statement
#ifdef __ebbrt__
    //if its back end, send the root val to home node
    ebbrt::kprintf("RecieveMessage: BM sending local val %d \n", GetRoot());
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id - 1;  // Send back with the original id
    dp.Get<uint32_t>() = GetRoot();
    SendMessage(nid, std::move(buf));
    return;
#else
    //if front end, call gather and reply with the send
    printf("ReceiveMessage: FE calling Gather!\n");
    auto vfg = Gather();
    auto v = ebbrt::when_all(vfg).Then([this, nid, id](auto vf){
        auto v = vf.Get();
        int gather_sum = this->GetRoot();
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
    auto it = root_->promise_map_.find(id);
    assert(it != root_->promise_map_.end());
    it->second.SetValue(val);
    root_->promise_map_.erase(it);
    return;
  }

}


std::vector<ebbrt::Future<int>> ebbrt::Counter::Gather(){
  std::vector<ebbrt::Future<int>> ret;
  //generic gather for all the nodes. Then home node will have more than one nids.
  //other nodes will only have one nid which is the home node's nid.
  uint32_t id;
  printf("gather nodelist size=%d\n", size());
  for(int i = 0; i < size(); i++){
    // if some of the nodes come up make a promise of a send n recieve pair
    // fufill the promise when gets back the node value
    auto nid = nlist(i);
    ebbrt::Promise<int> promise;
    auto f = promise.GetFuture();
    ret.push_back(std::move(f));
    {
      std::lock_guard<std::mutex> guard(lock_);
      id = 2 * (i+1);
      bool inserted;
      std::tie(std::ignore, inserted) =
	root_->promise_map_.emplace(id, std::move(promise));
      assert(inserted);
    }
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id + 1;  
    SendMessage(nid, std::move(buf));
  }
  return ret;
}

ebbrt::Future<int> ebbrt::Counter::ConsistentJoin(ebbrt::CounterRoot * root){
  //this join is a consistent join that hold untill the node itself knows its joined
  ebbrt::kprintf("joining the home node\n");
  ebbrt::Promise<int> promise;
  auto f = promise.GetFuture();
  int id;
  {
    std::lock_guard<std::mutex> guard(lock_);
    id = 10;
    bool inserted;
    std::tie(std::ignore, inserted) =
      root->promise_map_.emplace(id, std::move(promise));
    assert(inserted);
  }
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = 0; 
  ebbrt::kprintf("sending messages\n");
  SendMessage(root->nlist(0), std::move(buf));
  return f;
}

int ebbrt::Counter::GlobalVal(){
  //getting back a vector from gather, simply sum up the counts
  // if the nodelist is 0, which means non of th nodes are in the list yet, so just return the local value
  if (size() == 0){
    // if none of the nodes joined, then just do a simple local val
    ebbrt::kprintf("size is zero\n");
    return GetRoot();
  }
#ifdef __ebbrt__
  auto vfg = Gather();
  auto v = ebbrt::when_all(vfg).Block().Get();
  return v[0];
#else
  auto vfg = Gather();
  auto v = ebbrt::when_all(vfg).Block().Get();
  int gather_sum = this->GetRoot();
  for(uint32_t i = 0; i< v.size(); i++){
    gather_sum += v[i];
  }
  return gather_sum;
#endif
}

