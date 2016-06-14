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
  uint64_t granularity;
  uint64_t ps = ebbrt::pmem::kPageSize;
public:
  void setPage(ebbrt::Pfn page, uint64_t length) {
    page_ = page;
    pageLen = length;
    if (pageLen >= 1<<21){
      ps = 1<<21;
    }
    granularity = pageLen/ps;
  }
  void HandleFault(ebbrt::idt::ExceptionFrame* ef, uintptr_t faulted_address) override {
    ebbrt::kprintf("faulted address %#llx\n", faulted_address);
    //    ebbrt::kprintf("paddr = %#llx\n", page_.ToAddr());
    auto vpage = ebbrt::Pfn::Down(faulted_address);
    //    int c = 0;
    //    ebbrt::kprintf("page length %i\n", pageLen);
    auto page = page_;
    for (uint64_t i = 0; i < granularity; i++){
	ebbrt::vmem::MapMemory(vpage, page, ps);
	vpage = ebbrt::Pfn::Down(vpage.ToAddr() + ps);
	page = ebbrt::Pfn::Down(page.ToAddr() + ps);
	//	c++;
     }
    //     ebbrt::kprintf("called MapMemory for %i times.\n", c);
  }

};
// a function to be called to invoke events in other cores
void test(uintptr_t addr);
// this function use a for loop to fill the physical page with desireed value
void fillInValue(int value, int iteration, volatile uint32_t * ptr);
//this function use a for loop to lazly invoked the page fault handler
void lazyMap(int value, int iteration, int multiplier, volatile uint32_t * ptr);

void AppMain() { 
  int len = 9; // number of pages need to be allocated physical allocator
  int mul = 3; // multipler for how many times more virtual pages to be allocated
  auto  ps = ebbrt::pmem::kPageSize; // page size by default
  int iteration = ((int)ps*(1<<len))/4; // number of iterations for filling the page
  int v = 0xDEADBEEF; // value that can be filled into the page
  int unmap = 1;//a flag for unmaping

  printer->Print("MEMMAP BACKEND UP.\n"); 
  printer->Print("START TO ALLOCATE 1 PHYSICAL PAGES.\n");
  auto pfn = ebbrt::page_allocator->Alloc(len);
  auto pptr = (volatile uint32_t *)pfn.ToAddr();
  printer->Print("FILLING UP THE ADDRESS\n");
  fillInValue(v, iteration, pptr);

  printer->Print("START TO ALLOCATE VIRTUAL PAGE.\n");
  //create the instances of the page fault handler
  //and allocate the virtual memory pages which will hit page fault when dereferrenced
  auto pf = std::make_unique<NewPageFaultHandler>();
  pf->setPage(pfn, ps*(1<<len));
  auto vfn = ebbrt::vmem_allocator->Alloc(mul*(1<<len), std::move(pf));
  auto addr = vfn.ToAddr();
  auto ptr = (volatile uint32_t *)addr;
  lazyMap(v, iteration, mul, ptr);

  if (unmap == 1){
  printer->Print("BEGIN TO UNMAP THE VIRTUAL PAGE.\n");
  //start to unmap the virtual region by traversing the page talbes and delete the entries
  TraversePageTable(
       ebbrt::vmem::GetPageTableRoot(), addr, addr + mul*(1 << len)*ps, 0, 4,
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
  
  ptr = (volatile uint32_t *)addr;
  ebbrt::kbugon( (int)*ptr != v, "wrong value for remap the virtual region!!\n");
  // the following is for spawning different events on other cores.
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

void fillInValue(int value, int iteration, volatile uint32_t * ptr){
  for(int i = 0; i < iteration; i++){
    *ptr = value;
    ptr++;
  }

}

void lazyMap(int value, int iteration,int multiplier,  volatile uint32_t *ptr){
  for(int i = 0; i < multiplier * iteration; i++){
    volatile auto val = *ptr;
    ebbrt::kbugon( (int)val != value, "wrong value in the virtual memory!!\n");
    ptr++;
  }

}
