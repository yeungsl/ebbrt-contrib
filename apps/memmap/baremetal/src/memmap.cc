//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Printer.h"
#include <ebbrt/VMemAllocator.h>
#include <ebbrt/VMem.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/MemMap.h>
#include <ebbrt/PMem.h>


class NewPageFaultHandler : public ebbrt::VMemAllocator::PageFaultHandler {
  ebbrt::Pfn page_;
  uint64_t pageLen;
public:
  void setPage(ebbrt::Pfn page, uint64_t length) {
    page_ = page;
    pageLen = length;
  }
  void HandleFault(ebbrt::idt::ExceptionFrame* ef, uintptr_t faulted_address) override {
    auto vpage = ebbrt::Pfn::Down(faulted_address);
    ebbrt::vmem::MapMemory(vpage, page_, pageLen);

  }
};

void AppMain() { 
  int l = 3;
  int mul = 3;
  auto ps = ebbrt::pmem::kPageSize;
  int iteration = ((int)ps*(1<<l))/4;
  int v = 0xDEADBEEF;


  printer->Print("MEMMAP BACKEND UP.\n"); 
  printer->Print("START TO ALLOCATE 1 PHYSICAL PAGES.\n");
  auto pfn = ebbrt::page_allocator->Alloc(l);
  auto pptr = (volatile uint32_t *)pfn.ToAddr();

  ebbrt::kprintf("physical address allocated 0x%lx\n", (uintptr_t)pptr);
  printer->Print("FILLING UP THE ADDRESS\n");
  ebbrt::kprintf("interations: %i\n", iteration);
  for(int i = 0; i < iteration; i++){
    *pptr = v;
    //ebbrt::kprintf("i: %d, address 0x%lx has value 0x%lx\n",i, (uintptr_t)pptr, *pptr);
    pptr++;
  }
  
  printer->Print("START TO ALLOCATE VIRTUAL PAGE.\n");
  //dont know how to insert page fault handler
  auto pf = std::make_unique<NewPageFaultHandler>();
  pf->setPage(pfn, ps*(1<<l));
  auto vfn = ebbrt::vmem_allocator->Alloc(mul, std::move(pf));

  auto addr = vfn.ToAddr();
  auto ptr = (volatile uint32_t *)addr;
  ebbrt::kprintf("Virtual address allocated 0x%lx\n", (uintptr_t)ptr);
  auto counter = 0;
  for(int i = 0; i < mul * iteration; i++){
    volatile auto val = *ptr;
    //ebbrt::kprintf("Read value: 0x%lx\n", val);  
    val++;
    ptr++;
    counter++;
  }
  ebbrt::kprintf("final iteration of the virtual region: %i\n", counter);
}

