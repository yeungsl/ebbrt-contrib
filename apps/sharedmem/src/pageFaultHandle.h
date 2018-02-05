#include <ebbrt/native/VMemAllocator.h>
#include <ebbrt/native/VMem.h>
#include <ebbrt/native/PageAllocator.h>
#include <ebbrt/native/MemMap.h>
#include <ebbrt/native/Cpu.h>
#include <ebbrt/native/PMem.h>
#include "RemoteMem.h"


class NewPageFaultHandler : public ebbrt::VMemAllocator::PageFaultHandler {
  uint64_t pageLen;
  int value;
  uint64_t granularity;
  uint64_t iteration;
  int numberOfPages;
  uint64_t ps = ebbrt::pmem::kPageSize;
  ebbrt::Pfn pfn_;
  ebbrt::EbbRef<RemoteMemory> rm;
  int r;
public:
  void setPage(int l, int v, ebbrt::Pfn pfn, ebbrt::EbbRef<RemoteMemory> rm, int reply ) {
    r = reply;
    rm = rm;
    pfn_ = pfn;
    numberOfPages = l;
    pageLen = ps*(1 << l);
    value = v;
    iteration = ps*(1<<l)/4;
    if (pageLen >= 1<<21){
      ps = 1<<21;
    }
    granularity = pageLen/ps;
  }
  void HandleFault(ebbrt::idt::ExceptionFrame* ef, uintptr_t faulted_address) override {
    ebbrt::kprintf_force("faulted address %#llx, desired page  %d, value 0x%x, on core %d\n", faulted_address, numberOfPages, value, (size_t)ebbrt::Cpu::GetMine());
    // convert the pfn into pointer for conveniency
    auto pptr = (volatile uint32_t*) pfn_.ToAddr();

    // query to see if it is master or follower
    /* trying to move this logic out side
    auto f_c = rm->QueryMaster(OWNERSHIP);
    // block on the reply
    // NOTICE: should not be blocking, the proper way is to drop the lock and mark this handling failed
    auto reply = f_c.Block().Get();
    */

    ebbrt::kprintf_force("WHAT IS THE REPLY %d \n", r);
    if (r == MASTER){
      ebbrt::kprintf_force("RECEIVED MASTER REPLY\n");
      // filling up the page
      for(uint64_t i = 0; i < iteration; i++){
        *pptr = value;
        pptr++;
      }
      auto pptr_ = (volatile uint32_t * )pfn_.ToAddr();
      // cach up the page into Remote Mem
      rm->cachePage(pageLen, pptr_, iteration);
    }else{
      ebbrt::kprintf_force("RECEIVED FOLLOWER REPLY \n");
      // is follower, so ask for the page
      auto f_get = rm->QueryMaster(PAGE);
      auto reply = f_get.Block().Get();
      if (reply == SUCCESS){
	// get the page sucessfully, fetch the page from the cache
        rm->fetchPage(pptr);
      }else{
        ebbrt::kabort("FAIL TO GET A PAGE");
      }
    }
    auto vpage = ebbrt::Pfn::Down(faulted_address);
    auto page = pfn_;
    // MAP THE PAGE RETURN
   for (uint64_t i = 0; i < granularity; i++){
      ebbrt::vmem::MapMemory(vpage, page, ps);
      vpage = ebbrt::Pfn::Down(vpage.ToAddr() + ps);
      page = ebbrt::Pfn::Down(page.ToAddr() + ps);
     }
  }
};



