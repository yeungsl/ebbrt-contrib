#include "Membership.h"

EBBRT_PUBLISH_TYPE(,Membership);



//void Membership::Init(){return;}

void Membership::ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer){
  auto dp = buffer->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  auto str_nid = nid.ToString();
  addTo(nid);
  switch(id){

  case CONNECT:
    {
      std::stringstream ss;
      ss << str_nid << ": id " << id << " GOT CONNECT QUERY" << std::endl;
      ebbrt::kprintf(ss.str().c_str());
#ifdef __ebbrt__
      //greb ip address
      std::array<uint8_t, 4> ar_ip = {0, 0, 0, 0};
      for(int i = 0; i < 4; i++){
        ar_ip[i] = dp.Get<uint8_t>();
      }
      auto dst_nid = ebbrt::Messenger::NetworkId(std::string(reinterpret_cast<const char*>(ar_ip.data()), 4));
      //send a message
      auto buf = FufillOwnership(FOLLOWER);
      std::stringstream ss_2;
      ss_2 << "Telling " <<  dst_nid.ToString() << " WITH ID " << FOLLOWER  << std::endl;
      ebbrt::kprintf(ss_2.str().c_str());
      SendMessage(dst_nid, std::move(buf));
#else
      if (size() == 1){
        std::stringstream s_debug;
        s_debug << "REPLY " << str_nid << " MASTER" << std::endl;
        ebbrt::kprintf(s_debug.str().c_str());
        auto buf = FufillOwnership(MASTER);
        SendMessage(nid, std::move(buf));
      }else{
        std::stringstream s_debug;
        s_debug << "REPLY " << str_nid << " FOLLOWER" << std::endl;
        ebbrt::kprintf(s_debug.str().c_str());
        auto ow_buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t)*2);
        auto ow_dp = ow_buf->GetMutDataPointer();
        ow_dp.Get<uint32_t>() = CONNECT;
        char dst[20];
        std::strcpy(dst, nid.ToString().c_str());
        auto ip = std::strtok(dst, ".");
        while(ip != NULL){
          ow_dp.Get<uint8_t>() = std::atoi(ip);
          ip = std::strtok(NULL, ".");
        }
        ebbrt::kprintf("SENDING MSG TO %s\n", nodelist[0].ToString().c_str());
        SendMessage(nodelist[0], std::move(ow_buf));
      }
#endif

      break;
    }
  case FUFILL:
    {
      std::stringstream ss_f;
      ss_f << str_nid << ": id " << id << " GOT FUFILL OWNERSHIP QUERY" << std::endl;
      ebbrt::kprintf(ss_f.str().c_str());
      auto fufill_v = dp.Get<uint32_t>();
      promise->SetValue(fufill_v);
      {
        std::lock_guard<std::mutex> guard(lock_);
        assert(!ms_promise);
        ms_promise = true;
      }
      break;
    }
  }
}

std::unique_ptr<ebbrt::MutUniqueIOBuf>  Membership::FufillOwnership(int job){
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t)*3);
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = FUFILL;
  dp.Get<uint32_t>() = job;
  return buf;
}

ebbrt::Future<int> Membership::Query(ebbrt::Messenger::NetworkId nid){
  ebbrt::kprintf("QUERY TO %s\n", nid.ToString().c_str());
  ebbrt::kprintf("THE MB OBJECT WITH EBBID %d CREATED\n", kMemberEbbId);
  {
    std::lock_guard<std::mutex> guard(lock_);
    //std::unique_ptr<ebbrt::Promise<int>> p2 = std::make_unique<ebbrt::Promise<int>>();
    //promise.swap(std::move(std::make_unique<ebbrt::Promise<int>>()));
    promise.release();
    promise = std::make_unique<ebbrt::Promise<int>>();

    assert(ms_promise);
    ms_promise = false;
  }
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = CONNECT;
  SendMessage(nid, std::move(buf));
  return promise->GetFuture();

}


