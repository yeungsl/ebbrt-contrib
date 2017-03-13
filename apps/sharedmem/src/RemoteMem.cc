#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/IOBuf.h>
#include "RemoteMem.h"

RemoteMemory &
RemoteMemory::HandleFault(ebbrt::EbbId id){
  {
    // Trying to find if registered in the local Id map
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    if (found) {
      // If found the cache reference and return the rep pointer
      auto& r = *boost::any_cast<RemoteMemory*>(accessor->second);
      ebbrt::EbbRef<RemoteMemory>::CacheRef(id, r);
      return r;
    }
  }
  // join with home node, found in global_id_map
#ifdef __ebbrt__
  ebbrt::EventManager::EventContext context;
  auto f = ebbrt::global_id_map->Get(id);
  RemoteMemory* p;
  f.Then([&f, &context, &p, id](ebbrt::Future<std::string> inner) {
    p = new RemoteMemory();
    p->addTo(ebbrt::Messenger::NetworkId(inner.Get()));
    ebbrt::event_manager->ActivateContext(std::move(context));
  });
  ebbrt::event_manager->SaveContext(context);
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<RemoteMemory>::CacheRef(id, *p);
    return *p;
  }
#else
  RemoteMemory* p;
  ebbrt::global_id_map->Set(id, ebbrt::messenger->LocalNetworkId().ToBytes());
  p = new RemoteMemory();
  ebbrt::EbbRef<RemoteMemory>::CacheRef(id, *p);
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<RemoteMemory>::CacheRef(id, *p);
    return *p;
  }
#endif
  delete p;
  // retry reading
  ebbrt::LocalIdMap::ConstAccessor accessor;
  ebbrt::local_id_map->Find(accessor, id);
  auto& r = *boost::any_cast<RemoteMemory*>(accessor->second);
  ebbrt::EbbRef<RemoteMemory>::CacheRef(id, r);
  return r;
}


ebbrt::Future<int> RemoteMemory::ConsistentJoin(){
  //this join is a consistent join that hold untill the node itself knows its joined
  //ebbrt::kprintf("joining the home node\n");
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

void RemoteMemory::sendPage(uint64_t len, volatile uint32_t * pptr, uint64_t iteration){
  auto buffer = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t)+sizeof(uint64_t)+sizeof(uint64_t)+len);
  ebbrt::kprintf("uint32_t = %d, len = %d, loop = %d\n", sizeof(uint64_t), len, iteration);
  auto dp = buffer->GetMutDataPointer();
  dp.Get<uint32_t>() = 8;
  dp.Get<uint64_t>() = len;
  dp.Get<uint64_t>() = iteration;
  for (uint64_t i = 0; i < iteration; i++){
    dp.Get<uint32_t>() = *pptr;
    //ebbrt::kprintf("BM: value read into buffer 0x%llx\n", *pptr);
    pptr++;
  }
  SendMessage(nlist(0), std::move(buffer));
}

ebbrt::Future<int> RemoteMemory::queryPage(){
  if (size() == 1){
    ebbrt::Promise<int> promise;
    auto f = promise.GetFuture();
    int id;
    {
      std::lock_guard<std::mutex> guard(lock_);
      id = 8;
      bool inserted;
      std::tie(std::ignore, inserted) =
	promise_map_.emplace(id, std::move(promise));
      assert(inserted);
    }
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id + 1; 
    SendMessage(nlist(0), std::move(buf));
    return f;
  }
  else{
    ebbrt::kabort("no home node");
  }
}

void RemoteMemory::getPage(volatile uint32_t* pptr){
  if(tem_buffer.size() > 0){
    ebbrt::kprintf("iteration is %d\n", tem_buffer[1]);
    for (uint64_t i = 0; i < tem_buffer[1]; i++){
      *pptr = tem_buffer[i+2];
      //ebbrt::kprintf("setting physical page value 0x%x\n", *pptr);
      pptr++;
    }
  }else{
    ebbrt::kabort("did not get a page");
  }
}

void RemoteMemory::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
			 std::unique_ptr<ebbrt::IOBuf>&& buffer){
  auto dp = buffer->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  ebbrt::kprintf("id is %d\n", id);

  if(id == 0){
    addTo(nid);
    ebbrt::kprintf("node list length %d\n", nodelist.size());
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    if (size() == 1){
      dp.Get<uint32_t>() = 10;
    }else{
      dp.Get<uint32_t>() = 11;
    }
    SendMessage(nid, std::move(buf));
    return;
  }

  if (id == 8){
    std::lock_guard<std::mutex> guard(lock_);
    auto len = dp.Get<uint64_t>();
    auto loop = dp.Get<uint64_t>();
    ebbrt::kprintf("page recieved has length %d \n", loop);
    tem_buffer.push_back(len);
    tem_buffer.push_back(loop);
    for (uint64_t i = 0; i < loop; i++){
      tem_buffer.push_back(dp.Get<uint32_t>());
      //ebbrt::kprintf("FE: value get from buffer 0x%llx\n", tem_buffer[i+2]);
    }
#ifdef __ebbrt__
    auto it = promise_map_.find(id);
    assert(it != promise_map_.end());
    it->second.SetValue(id);
    promise_map_.erase(it);
#endif
    return;
  }

  if (id == 9){
    std::lock_guard<std::mutex> guard(lock_);
    auto buffer = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t)+sizeof(uint64_t)+sizeof(uint64_t)+tem_buffer[0]);
    auto dp = buffer->GetMutDataPointer();
    dp.Get<uint32_t>() = 8;
    dp.Get<uint64_t>() = tem_buffer[0];
    dp.Get<uint64_t>() = tem_buffer[1];
    for (uint64_t i = 0; i < tem_buffer[1]; i++){
      dp.Get<uint32_t>() = tem_buffer[i+2];
    }
    SendMessage(nid, std::move(buffer));
    return;
  }
  if(id >= 10 && id < 12){
    std::lock_guard<std::mutex> guard(lock_);
    auto it = promise_map_.find(10);
    assert(it != promise_map_.end());
    it->second.SetValue(id);
    promise_map_.erase(it);
    return;
  }
}
