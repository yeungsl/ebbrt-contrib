#include "RemoteMem.h"
EBBRT_PUBLISH_TYPE(, RemoteMemory);

#ifdef __ebbrt__
ebbrt::Future<int> RemoteMemory::QueryMaster(){
  // This function talks to the front end and get responses
  ebbrt::kprintf("value of promise_aval: %d\n", promise_aval);
  return ms->Query(FrontendId);

}

void RemoteMemory::sendPage(ebbrt::Messenger::NetworkId dst){
  // for the master to send page to the desired NID
  // should be back end only!!!

  if(cached){ebbrt::kprintf("CACHED PROB\n");}

  auto buffer = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t) + sizeof(uint32_t)+sizeof(uint64_t)+sizeof(uint64_t)+page_len);
  ebbrt::kprintf("size of buffer = %d, len = %d, loop = %d, pfn = %x\n", sizeof(buffer), page_len, page_iteration, pfn_);
  auto pptr = (volatile uint32_t*) pfn_.ToAddr();
  auto dp = buffer->GetMutDataPointer();
  // command for receive message
  dp.Get<uint32_t>() = FUFILL;
  dp.Get<uint64_t>() = page_len;
  dp.Get<uint64_t>() = page_iteration;
  for (uint64_t i = 0; i < page_iteration; i++){
    dp.Get<uint32_t>() = *pptr;
    pptr++;
  }
  ebbrt::kprintf("destination nid: %s\n", dst.ToString().c_str());

  SendMessage(dst, std::move(buffer));
}

void RemoteMemory::cachePage(uint64_t len, ebbrt::Pfn pfn, uint64_t iteration){
  // MASTER ONLY !!!!!
  // cache the page
  // NOTICE: the follower might be asking page before the page is cached, therefore should drop the request if not cached
  page_len = len;
  pfn_ = pfn;
  page_iteration = iteration;
  cached = true;
  return;
}

ebbrt::Pfn RemoteMemory::fetchPage(){
  // FOLLOWER ONLY!!!!
  ebbrt::kprintf("START TO FETCH THE PAGE ON CORE %d\n", (size_t)ebbrt::Cpu::GetMine());

  {
    std::lock_guard<std::mutex> guard(lock_);
    //std::unique_ptr<ebbrt::Promise<int>> p2 = std::make_unique<ebbrt::Promise<int>>();
    //promise.swap(std::move(std::make_unique<ebbrt::Promise<int>>()));
    promise.release();
    promise = std::make_unique<ebbrt::Promise<int>>();
    ebbrt::kprintf("value of promise_aval: %d\n", promise_aval);
    assert(promise_aval);
    promise_aval = false;
  }
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = SEND;
  ebbrt::kprintf("SENDING A FETCH TO %s\n", ms->Master().ToString().c_str());
  SendMessage(ms->Master(), std::move(buf));
  auto f_get = promise->GetFuture();
  auto reply = f_get.Block().Get();

  if (reply == SUCCESS){

    return pfn_;
  }
  else{
    ebbrt::kabort("DID NOT RECIEVED THE PAGE");
  }
}
#endif


void RemoteMemory::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
			 std::unique_ptr<ebbrt::IOBuf>&& buffer){

  auto dp = buffer->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  auto str_nid = nid.ToString();

  switch(id){

  case FUFILL:
    // this is for fufilling the transactions
    // two types of transaction:
    // 1. checking ownership
    // 2. recieving a page
    {
      std::stringstream ss_f;
      ss_f << str_nid << ": id " << id << " GOT FUFILL QUERY" << std::endl;
      ebbrt::kprintf(ss_f.str().c_str());
#ifdef __ebbrt__
        pfn_ = ebbrt::page_allocator->Alloc(0);
        auto pptr = (volatile uint32_t *) pfn_.ToAddr();
        page_len = dp.Get<uint64_t>();
        page_iteration = dp.Get<uint64_t>();
        ebbrt::kprintf("GET PAGE SUCCESFULL ON CORE %d\n", (size_t)ebbrt::Cpu::GetMine());
        for(uint64_t i = 0; i < page_iteration; i++){
          *pptr = dp.Get<uint32_t>();
          pptr++;
        }
        promise->SetValue(SUCCESS);
#else
        ebbrt::kabort("FRONT END SHOULD NOT RECIEVE THIS \n");
#endif
      break;
    }


  case SEND:
    // responding for sending request from front end
    // parse out the nid and send the page
    // NOTICE: if not master, or the page is not cached
    //         kabort will be called
    // OPTION: could store the reqeust and send after page is cached.
    //         or just drop the request, wait for the request to be send again.
    {
    std::stringstream ss_f;
    ss_f << str_nid << ": id " << id << " GOT SEND REQEUST" << std::endl;
    ebbrt::kprintf(ss_f.str().c_str());
#ifdef __ebbrt__
    sendPage(nid);
#else
    ebbrt::kabort("FRONT END SHOULD NOT RECIEVE THIS\n");
#endif
    break;
    }
  return;
  }
}
