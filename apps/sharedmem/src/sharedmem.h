#include <ebbrt/native/VMemAllocator.h>
#include <ebbrt/native/VMem.h>
#include <ebbrt/native/PageAllocator.h>
#include <ebbrt/native/MemMap.h>
#include <ebbrt/native/PMem.h>
#include "RemoteMem.h"


class NewPageFaultHandler : public ebbrt::VMemAllocator::PageFaultHandler {
  uint64_t pageLen;
  int value;
  uint64_t granularity;
  uint64_t iteration;
  int numberOfPages;
  uint64_t ps = ebbrt::pmem::kPageSize;
public:
  void setPage(int l, int v) {
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
    ebbrt::kprintf("faulted address %#llx, desired page  %d, value 0x%x\n", faulted_address, numberOfPages, value);
    auto pfn = ebbrt::page_allocator->Alloc(numberOfPages);
    auto pptr = (volatile uint32_t*) pfn.ToAddr();
    auto rm = ebbrt::EbbRef<RemoteMemory>(kRemoteMEbbId);
    rm->size();
    auto f_c = rm->ConsistentJoin();
    auto reply = f_c.Block().Get();
    ebbrt::kprintf("BM: reply is %d, pointer size %d\n", reply,  ((int)ps*(1<<pageLen))/4);
    if (reply == 10){
      for(uint64_t i = 0; i < iteration; i++){
	*pptr = value;
	pptr++;
      }
      auto pptr_ = (volatile uint32_t * )pfn.ToAddr();
      rm->sendPage(pageLen, pptr_, iteration);
    }
    if (reply == 11){
      auto f = rm->queryPage();
      auto r = f.Block().Get();
      ebbrt::kprintf("BM: reply is %d\n", r);
      if (r == 8){
	rm->getPage(pptr);
      }
    }
    auto vpage = ebbrt::Pfn::Down(faulted_address);
    auto page = pfn;
    for (uint64_t i = 0; i < granularity; i++){
	ebbrt::vmem::MapMemory(vpage, page, ps);
	vpage = ebbrt::Pfn::Down(vpage.ToAddr() + ps);
	page = ebbrt::Pfn::Down(page.ToAddr() + ps);
     }
  }
};



