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


ebbrt::Future<int> RemoteMemory::QueryMaster(int command){
  // This function talks to the front end and get responses

  if (command != PAGE && command != OWNERSHIP ){
    // ONLY TWO COMMAND HAS BEEN IMPLMENTED
    ebbrt::kabort("UNSEEN COMMAND\n");
  }
  if (command == PAGE){
    // for the back end messenger interface to connect to each other
    // TODO: implement a TCP handler instead of using messenger
#ifdef __ebbrt__
      int p = 32765;
      ebbrt::kprintf("Start Listening on port %d\n", p);
      ebbrt::messenger->StartListening(p);
#endif
  }
  // making a transaction map and send message to front end
  ebbrt::Promise<int> promise;
  auto f = promise.GetFuture();
  {
    std::lock_guard<std::mutex> guard(lock_);
    auto it = promise_map_.find(command);
    assert(it == promise_map_.end());
    bool inserted;
    std::tie(std::ignore, inserted) =
      promise_map_.emplace(command, std::move(promise));
    assert(inserted);
  }
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = command; 
  SendMessage(nodelist[0], std::move(buf));
  return f;
}

void RemoteMemory::sendPage(ebbrt::Messenger::NetworkId dst){
  // for the master to send page to the desired NID
  // should be back end only!!!

  auto buffer = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t) + sizeof(uint32_t)+sizeof(uint64_t)+sizeof(uint64_t)+tem_buffer[0]);
  ebbrt::kprintf("size of buffer = %d, len = %d, loop = %d\n", sizeof(buffer), tem_buffer[0], tem_buffer[1]);
  auto dp = buffer->GetMutDataPointer();
  // command for receive message
  dp.Get<uint32_t>() = FUFILL;
  dp.Get<uint32_t>() = PAGE;
  dp.Get<uint64_t>() = tem_buffer[0];
  dp.Get<uint64_t>() = tem_buffer[1];
  for (uint64_t i = 0; i < tem_buffer[1]; i++){
    dp.Get<uint32_t>() = tem_buffer[i+2];
    // ebbrt::kprintf("BM: value read into buffer 0x%llx\n", tem_buffer[i+2]);
  }
  ebbrt::kprintf("destination nid: %s\n", dst.ToString().c_str());  

  SendMessage(dst, std::move(buffer));
}

void RemoteMemory::cachePage(uint64_t len, volatile uint32_t * pptr, uint64_t iteration){
  // MASTER ONLY !!!!!
  // cache the page 
  // NOTICE: the follower might be asking page before the page is cached, therefore should drop the request if not cached
  ebbrt::kprintf("Cached the paged\n");
  tem_buffer.push_back(len);
  tem_buffer.push_back(iteration);
  for (uint64_t i = 0; i < iteration; i++){
    tem_buffer.push_back(*pptr);
    //ebbrt::kprintf("BM: value read into buffer 0x%llx\n", *pptr);
    pptr++;
  }
  ebbrt::kprintf("size of tem_buffer = %d, len = %d, loop = %d\n", tem_buffer.size(), len, iteration);
  cached = true;
  // same purpose for establishing the connection through messenger
  // TODO: new page sending interface
#ifdef __ebbrt__
      int p = 32765;
      ebbrt::kprintf("Start Listening on port %d\n", p);
      ebbrt::messenger->StartListening(p);
#endif
  return;
}

void RemoteMemory::fetchPage(volatile uint32_t* pptr){
  // if page arrives then wirte the page from the cache
  // FOLLOWER ONLY!!!!

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
  switch(id){

  case OWNERSHIP:
    {
      auto ow_buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t) * 3);
      auto ow_dp = ow_buf->GetMutDataPointer();
      addTo(nid);
      ow_dp.Get<uint32_t>() = FUFILL;
      ow_dp.Get<uint32_t>() = OWNERSHIP;
      if (size() == 1) {
	ow_dp.Get<uint32_t>() = MASTER;
      }else{
	ow_dp.Get<uint32_t>() = FOLLOWER;
      }
      SendMessage(nid, std::move(ow_buf));
      break;
    }

  case FUFILL:
    // this is for fufilling the transactions
    // two types of transaction:
    // 1. checking ownership
    // 2. recieving a page
    {
      auto trans_num = dp.Get<uint32_t>();
      switch(trans_num){
      case OWNERSHIP:
	{
	  auto it = promise_map_.find(OWNERSHIP);
	  assert(it != promise_map_.end());
	  auto fufill_v = dp.Get<uint32_t>();
	  it->second.SetValue(fufill_v);
	  promise_map_.erase(it);
	  break;
	}
      case PAGE:
	{
	  auto len = dp.Get<uint64_t>();
	  auto iteration = dp.Get<uint64_t>();
	  tem_buffer.push_back(len);
	  tem_buffer.push_back(iteration);
	  for(uint64_t i = 0; i < iteration; i++){
	    tem_buffer.push_back(dp.Get<uint32_t>());
	  }
	  auto it = promise_map_.find(PAGE);
	  assert(it != promise_map_.end());
	  it->second.SetValue(SUCCESS);
	  promise_map_.erase(it);
	  break;
	}
      }
      break;
    }

  case PAGE:
    // responding for recieving a query for page
    // get the nid of the requester and send to master
    // FRONT END ONLY!!
    // OPTION: could send the master's nid to the requester
    //        and let it to request the page from master.
    {
      auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
      auto dp = buf->GetMutDataPointer();
      dp.Get<uint32_t>() = SEND;
      char dst[20];
      std::strcpy(dst, nid.ToString().c_str());
      auto ip = std::strtok(dst, ".");
      while(ip != NULL){
	dp.Get<uint8_t>() = std::atoi(ip);
	ip = std::strtok(NULL, ".");
      }
      SendMessage(nodelist[0], std::move(buf));
      break;
    }

  case SEND:
    // responding for sending request from front end
    // parse out the nid and send the page
    // NOTICE: if not master, or the page is not cached
    //         kabort will be called
    // OPTION: could store the reqeust and send after page is cached.
    //         or just drop the request, wait for the request to be send again.

#ifdef __ebbrt__
    if (cached){
	std::array<uint8_t, 4> ar_ip = {0, 0, 0, 0};
	for(int i = 0; i < 4; i++){
	  ar_ip[i] = dp.Get<uint8_t>();
	}
	auto dst_nid = ebbrt::Messenger::NetworkId(std::string(reinterpret_cast<const char*>(ar_ip.data()), 4));
	sendPage(dst_nid);
    }else{
      ebbrt::kabort("UNIMPLEMENTED\n");
    }
#else
    ebbrt::kabort("FRONT END SHOULD NOT RECIEVE THIS\n");
#endif
    break;
  }
  return;
}
