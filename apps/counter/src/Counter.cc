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

using namespace Counter;
EBBRT_PUBLISH_TYPE(, SharedCounter);


#ifndef __ebbrt__
ebbrt::Future<void>
Counter::SharedCounter::Init()
{

  return ebbrt::global_id_map->Set(kCounterEbbId, ebbrt::messenger->LocalNetworkId().ToBytes())
      .Then([](ebbrt::Future<void> f) {
        f.Get();
        return ;

      });


}
#endif

Counter::SharedCounter::Root::Root(ebbrt::EbbId id) : myId_(id), theRep_(NULL)  {
  data_ = ebbrt::global_id_map->Get(id).Share();
}

Counter::SharedCounter *
Counter::SharedCounter::Root::getRep_BIN() {
  std::lock_guard<std::mutex> lock{lock_};
  if (theRep_) return theRep_; // implicity drop lock

  if (theRep_ == NULL)  { 
    // now that we are ready create the rep if necessary
    theRep_ = new SharedCounter(this);
  }
  return theRep_;      // implicity drop lock
}


// I am modeling this based on the following example from TBB doc
// http://www.threadingbuildingblocks.org/docs/help/reference/containers_overview/concurrent_hash_map_cls/concurrent_operations.htm
//  extern tbb::concurrent_hash_map<Key,Resource,HashCompare> Map;
// void ConstructResource( Key key ) {
//        accessor acc;
//        if( Map.insert(acc,key) ) {
//                // Current thread inserted key and has exclusive access.
//                ...construct the resource here...
//        }
//        // Implicit destruction of acc releases lock
// }

// void DestroyResource( Key key ) {
//        accessor acc;
//        if( Map.find(acc,key) ) {
//                // Current thread found key and has exclusive access.
//                ...destroy the resource here...
//                // Erase key using accessor.
//                Map.erase(acc);
//        }
// }
Counter::SharedCounter & 
Counter::SharedCounter::HandleFault(ebbrt::EbbId id) {
 retry:
  {
    ebbrt::LocalIdMap::ConstAccessor rd_access;
    if (ebbrt::local_id_map->Find(rd_access, id)) {
      // COMMON: "HOT PATH": NODE HAS A ROOT ESTABLISHED
      // EVERYONE MUST EVENTUALLY GET HERE OR THROW AND EXCEPTION
      // Found the root get a rep and return
      auto &root = *boost::any_cast<Root *>(rd_access->second);
      rd_access.release();  // drop lock
      // NO LOCKS;
      auto &rep = *(root.getRep_BIN());   // this may block internally
      ebbrt::EbbRef<SharedCounter>::CacheRef(id, rep);
      // The sole exit path out of handle fault
      root.getRep_BIN()->join();
      return rep; // drop rd_access lock by exiting outer scope
    }
  } 
  // failed to find root: NO LOCK held and we need to establish a root for this node
  ebbrt::LocalIdMap::Accessor wr_access;
  if (ebbrt::local_id_map->Insert(wr_access, id)) {
    // WRITE_LOCK HELD:  THIS HOLDS READERS FROM MAKING PROGESS
    //                   ONLY ONE WRITER EXITS
    Root *root = new Root(id);
    wr_access->second = root;
    wr_access.release(); // WE CAN NOW DROP THE LOCK and retry as a normal reader
  }
  // NO LOCKS HELD
  // if we failed to insert then someone else must have beat us
  // and is the writer and will eventuall fill in the entry.
  // all we have to do is retry a read on the entry
  goto retry;
}



void Counter::SharedCounter::join(){
#ifdef __ebbrt__
  uint32_t id = 0;
  auto f = myRoot_->getString();
  f.Block();
  auto nid = ebbrt::Messenger::NetworkId(f.Get());
  ebbrt::kprintf("1BM : got nid: %s\n", nid.ToString().c_str());
  ebbrt::kprintf("joining the home node\n");
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = id;  // Ping messages are odd
  SendMessage(nid, std::move(buf));
#endif
}

void Counter::SharedCounter::ReceiveMessage(ebbrt::Messenger::NetworkId nid, 
				       std::unique_ptr<ebbrt::IOBuf>&& buffer){
  auto dp = buffer->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  ebbrt::kprintf("id is %d\n", id);
  if(id == 0){
    printf("0 FE: Join!\n");
    Counter::theCounter->addTo(nid);
    return;
  }
  if (id & 1){
    // Received Ping
    ebbrt::kprintf("1 BM: local val %d \n", Counter::theCounter->val());
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id - 1;  // Send back with the original id
    dp.Get<uint32_t>() = Counter::theCounter->val();
    SendMessage(nid, std::move(buf));
    return;
  }
  if (!(id & 1)){
    auto val = dp.Get<uint32_t>();
    printf("0 FE: get val %d\n", val);
    // Received Pong, lookup in the hash table for our promise and set it
    std::lock_guard<std::mutex> guard(lock_);
    auto it = promise_map_.find(id);
    assert(it != promise_map_.end());
    it->second.SetValue(val);
    promise_map_.erase(it);
    return;
  }
}



std::vector<ebbrt::Future<int>> Counter::SharedCounter::gather(){
  std::vector<ebbrt::Future<int>> ret;
#ifdef __ebbrt__
  ebbrt::kprintf("1BM: CALLING GATHER\n");
  /*
  ebbrt::kprintf("1BM : start gather\n");
  auto f = myRoot_->getString();
  f.Block();
  auto nid = ebbrt::Messenger::NetworkId(f.Get());
  ebbrt::kprintf("1BM : got nid: %s\n", nid.ToString().c_str());
  */
  return ret;
#else
  printf("0FE: CALLED GATHER\n");

  uint32_t id;
  if (SharedCounter::size() == 0){
    ebbrt::Promise<int> promise;
    auto f = promise.GetFuture();
    ret.push_back(std::move(f));
    printf("0FE: nothing in the node list yet\n");
    promise.SetValue(0);
    return ret;
  }

  for(int i = 0; i < SharedCounter::size(); i++){
    auto nid = SharedCounter::nlist(i);
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
    printf("gather id %d\n", id);
    SendMessage(nid, std::move(buf));
  }
  return ret;
#endif
}

