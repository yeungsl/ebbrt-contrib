//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Printer.h"
#include <ebbrt/VMemAllocator.h>
#include <ebbrt/VMem.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/MemMap.h>

class NewPageFaultHandler : public ebbrt::VMemAllocator::PageFaultHandler {
  ebbrt::Pfn page_;
public:
  void setPage(ebbrt::Pfn page) {
    page_ = page;
  }
  void HandleFault(ebbrt::idt::ExceptionFrame* ef, uintptr_t faulted_address) override {
    auto vpage = ebbrt::Pfn::Down(faulted_address);
    ebbrt::vmem::MapMemory(vpage, page_);

  }
};

void AppMain() { 
  //  const npages = 2;
  printer->Print("MEMMAP BACKEND UP.\n"); 
  printer->Print("START TO ALLOCATE 1 PHYSICAL PAGES.\n");
  auto pfn = ebbrt::page_allocator->Alloc(0);
  auto pptr = (volatile uint32_t *)pfn.ToAddr();

  ebbrt::kprintf("physical address allocated 0x%lx\n", (uintptr_t)pptr);
  printer->Print("FILLING UP THE ADDRESS\n");
  for(int i = 0; i < 1024; i++){
    *pptr = 3735928559;
    
    //ebbrt::kprintf("i: %d, address 0x%lx has value 0x%lx\n",i, (uintptr_t)pptr, *pptr);
    pptr++;
  }
  
  printer->Print("START TO ALLOCATE VIRTUAL PAGE.\n");
  //dont know how to insert page fault handler
  auto pf = std::make_unique<NewPageFaultHandler>();
  pf->setPage(pfn);
  auto vfn = ebbrt::vmem_allocator->Alloc(3, std::move(pf));

  auto addr = vfn.ToAddr();
  auto ptr = (volatile uint32_t *)addr;
  ebbrt::kprintf("Virtual address allocated 0x%lx\n", (uintptr_t)ptr);
  for(int i = 1; i < 3 * 1024; i++){
    auto val = *ptr;
    ebbrt::kprintf("Read value: 0x%lx\n", val);  
    ptr++;
  }
}

