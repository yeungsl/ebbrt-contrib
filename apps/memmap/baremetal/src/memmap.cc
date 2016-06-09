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
#include <ebbrt/VMem.h>


class NewPageFaultHandler : public ebbrt::VMemAllocator::PageFaultHandler {
  ebbrt::Pfn page_;
  uint64_t pageLen;
public:
  void setPage(ebbrt::Pfn page, uint64_t length) {
    page_ = page;
    pageLen = length;
  }
  void HandleFault(ebbrt::idt::ExceptionFrame* ef, uintptr_t faulted_address) override {
    ebbrt::kprintf("faulted address %#llx\n", faulted_address);
    ebbrt::kprintf("paddr = %#llx\n", page_.ToAddr());
    auto ps = ebbrt::pmem::kPageSize;
    auto vpage = ebbrt::Pfn::Down(faulted_address);
    int c = 0;
    ebbrt::kprintf("page length %i\n", pageLen);
    if (pageLen >= 1<<21){
      ps = 1<<21;
    }
    auto page = page_;
    for (uint64_t i = 0; i < (pageLen/ps); i++){
	ebbrt::vmem::MapMemory(vpage, page_, ps);
	vpage = ebbrt::Pfn::Down(vpage.ToAddr() + ps);
	page = ebbrt::Pfn::Down(page.ToAddr() + ps);
	c++;
     }
     ebbrt::kprintf("called MapMemory for %i times.\n", c);
  }

};

void test(uintptr_t addr);

void fillInValue(int value, int iteration, uintptr_t ptr);

void AppMain() { 
  int l = 9;
  int mul = 3;
  auto ps = ebbrt::pmem::kPageSize;
  int iteration = ((int)ps*(1<<l))/4;
  int v = 0xDEADBEEF;
  int unmap = 1;

  printer->Print("MEMMAP BACKEND UP.\n"); 
  printer->Print("START TO ALLOCATE 1 PHYSICAL PAGES.\n");
  auto pfn = ebbrt::page_allocator->Alloc(l);
  auto pptr = (volatile uint32_t *)pfn.ToAddr();

  ebbrt::kprintf("physical address allocated 0x%lx\n", (uintptr_t)pptr);
  printer->Print("FILLING UP THE ADDRESS\n");
  ebbrt::kprintf("interations: %i\n", iteration);
  auto pcounter = 0;
  ebbrt::kprintf("value filled: 0x%x\n", v);
  for(int i = 0; i < iteration; i++){
    *pptr = v;
    if (*pptr == 0XDEADBEEF){
      pcounter++;
    }
    pptr++;
  }
  ebbrt::kprintf("number of space filled: %i\n", pcounter);

  printer->Print("START TO ALLOCATE VIRTUAL PAGE.\n");
  //dont know how to insert page fault handler
  auto pf = std::make_unique<NewPageFaultHandler>();
  pf->setPage(pfn, ps*(1<<l));
  auto vfn = ebbrt::vmem_allocator->Alloc(mul*(1<<l), std::move(pf));

  auto addr = vfn.ToAddr();
  auto ptr = (volatile uint32_t *)addr;
  ebbrt::kprintf("Virtual address allocated 0x%lx\n", (uintptr_t)ptr);
  auto counter = 0;
  for(int i = 0; i < mul * iteration; i++){
    volatile auto val = *ptr;
    if (val == 0xDEADBEEF){
      counter++; 
    }
    ptr++;
  }
  ebbrt::kprintf("virtual region filled: %i\n", counter);
  ebbrt::kprintf("number of iteration: %i\n", mul*iteration);
  if (unmap == 1){
  printer->Print("BEGIN TO UNMAP THE VIRTUAL PAGE.\n");

  TraversePageTable(
		    ebbrt::vmem::GetPageTableRoot(), addr, addr + mul*(1 << l)*ps, 0, 4,
       [=](ebbrt::vmem::Pte& entry, uint64_t base_virt, size_t level) {
        kassert(entry.Present());
        entry.Clear();
        std::atomic_thread_fence(std::memory_order_release);
        asm volatile("invlpg (%[addr])" : : [addr] "r"(base_virt) : "memory");
      },
	[=](ebbrt::vmem::Pte& entry) {
        ebbrt::kprintf("Asked to unmap memory that wasn't mapped!\n");
        ebbrt::kabort();
        return false;
      });
  };
  

  ebbrt::kprintf("paddr = %#llx\n", pfn.ToAddr());
  pptr = (volatile uint32_t *)pfn.ToAddr();
  ebbrt::kprintf("value in physical address: 0x%x\n", *pptr);
  ebbrt::kprintf("addr = 0x%llx\n", addr);
  ptr = (volatile uint32_t *)addr;
  volatile auto val = *ptr;
  ebbrt::kprintf("Read value: 0x%x\n", val);
  //  auto f = [addr]() {
  //  ebbrt::kprintf("Hello from %u\n", (size_t)ebbrt::Cpu::GetMine());
  //  test(addr);
  //  test(addr);
  // };
  //ebbrt::event_manager->SpawnLocal(f);
  //  ebbrt::event_manager->SpawnRemote(f, 1);
}

void test(uintptr_t addr) {
  auto ptr = (volatile uint32_t *)addr;
  (void)*ptr;  
}

void fillInValue(int value, int iteration, uintptr_t ptr){
  for(int i = 0; i < iteration; i++){
    *pptr = v;
    if (*pptr == 0XDEADBEEF){
      pcounter++;
    }
    pptr++;
  }

}
