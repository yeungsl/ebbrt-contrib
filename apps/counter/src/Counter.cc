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
  auto f = myRoot_->getString();
  f.Block();
  auto nid = ebbrt::Messenger::NetworkId(f.Get());
  ebbrt::kprintf("1BM : got nid: %s\n", nid.ToString().c_str());

  ebbrt::kprintf("joining the home node\n");
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = 1;  // Ping messages are odd
  SendMessage(nid, std::move(buf));
#endif
  /*
  auto str = "1";
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char*>(buf->MutData()), len, "%s", str);
  */
}

void Counter::SharedCounter::ReceiveMessage(ebbrt::Messenger::NetworkId nid, 
				       std::unique_ptr<ebbrt::IOBuf>&& buffer){
  auto dp = buffer->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  ebbrt::kprintf("id is %d\n", id);

  if(id == 1){
    printf("0 FE: Join!\n");
    auto str = "Hello world";
    auto buf = ebbrt::MakeUniqueIOBuf(13);
    snprintf(reinterpret_cast<char*>(buf->MutData()), 13, "%s", str);
    SendMessage(nid, std::move(buf));

  }

  //recieve msg needed implementation
  if (id == 11){
    ebbrt::kprintf("1 BM: PING\n");
    // Received Ping
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id - 1;  // Send back with the original id
    ebbrt::kprintf("send message?\n");
    SendMessage(nid, std::move(buf));
  }
  if (id == 10){
    printf("0 FE: PONG\n");
    // Received Pong, lookup in the hash table for our promise and set it
    std::lock_guard<std::mutex> guard(lock_);
    auto it = promise_map_.find(id);
    assert(it != promise_map_.end());
    it->second.SetValue(id);
    promise_map_.erase(it);
  }
}
#ifndef __ebbrt__
ebbrt::Future<int> Counter::SharedCounter::gather(){
  /*
  ebbrt::kprintf("1BM : start gather\n");
  auto f = myRoot_->getString();
  f.Block();
  auto nid = ebbrt::Messenger::NetworkId(f.Get());
  ebbrt::kprintf("1BM : got nid: %s\n", nid.ToString().c_str());
  */
  ebbrt::kprintf("0 FE : start gather\n");
  uint32_t id = 10;
  ebbrt::Promise<int> promise;
  auto nid = SharedCounter::nlist(0);
  auto ret = promise.GetFuture();

  {
    std::lock_guard<std::mutex> guard(lock_);
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
  ebbrt::kprintf("in gather!!\n");
  return ret;
}
#endif