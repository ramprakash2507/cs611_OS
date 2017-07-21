#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;


void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
  kernel_mem_pool = _kernel_mem_pool;
  process_mem_pool = _process_mem_pool;
  shared_size = _shared_size; 
//  assert(false);
  Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
  page_directory = (unsigned long *)((process_mem_pool->get_frames(1))*PAGE_SIZE);
  unsigned long * page_table = (unsigned long *)((process_mem_pool->get_frames(1))*PAGE_SIZE);
  unsigned int entries = PAGE_SIZE/4;
  unsigned long address = 0;
  while(entries){
    page_table[PAGE_SIZE/4 - entries] = address | 3;  // attribute set to: supervisor level, read/write, present(011 in binary)
    address = address + 4096; // 4096 = 4kb
    entries--;
  }
  entries = PAGE_SIZE/4 - 1;
  page_directory[0] = ((unsigned long)page_table) | 3;
  while(entries){
    page_directory[PAGE_SIZE/4 - entries] = 0 | 2;
    entries--;
  }
  page_directory[1023] = ((unsigned long)page_directory) | 3;
  Console::puts("Constructed Page Table object\n");
  // assert(false);
}


void PageTable::load()
{
  current_page_table = this;
  write_cr3((unsigned long) page_directory);
  //assert(false);
  Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
  unsigned long cr0 = read_cr0();
  write_cr0(cr0 | 0x80000000);
  paging_enabled = 1;
  //assert(false);
  Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  bool legit = false;
  vmPool * it = current_page_table->vmHead;
  unsigned long address = read_cr2();
  int i=0;
  
  while(it != NULL){
    VMPool * itPool = (VMPool *)(it->pool);
     if(itPool->is_legitimate(address)){
        legit = true;
        break;
     }
    i++;
    it = it->next;
  }
  assert(legit);
  unsigned int err_code = _r->err_code;
  if(err_code & 1){
    Console::puts("No protection mechanism implemented");
    assert(false);
  }
  unsigned long mask = (err_code & 6) ^ 0x00000005;
  unsigned long * page_vrt_dir = (unsigned long *)((0xfffff000 | (address >> 20)) & 0xfffffffc);
  if(((*page_vrt_dir) & 1) == 0){
    unsigned long * page_table = (unsigned long *)((process_mem_pool->get_frames(1))*PAGE_SIZE);
    *page_vrt_dir = ((unsigned long)page_table) | 3;
  }
  unsigned long * pt_vrt_adr = (unsigned long *)((0xffc00000 | (address >> 10)) & 0xfffffffc);
  if((*pt_vrt_adr) & 1){
    Console::puts("address: ");Console::puti(address);Console::puts("\n");  
    assert(false);
  }else{
    unsigned long * page = (unsigned long *)((process_mem_pool->get_frames(1))*PAGE_SIZE);
    *pt_vrt_adr = ((unsigned long) page) | mask;
  }
  //assert(false);
  Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{
  if(vmHead == NULL){
      vmHead = &(_vm_pool->poolNode);
      vmHead->next = NULL;
  }
  if(vmTail != NULL){
      vmTail->next = &(_vm_pool->poolNode);
  }
  vmTail = &(_vm_pool->poolNode);
  vmTail->pool = _vm_pool;
  vmTail->next = NULL; 
  //assert(false);
  Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no) {
  //assert(false);
    
  unsigned long * pt_vrt_adr = (unsigned long *)(0xffc00000 | (_page_no << 2));
  if((*pt_vrt_adr) & 1){
    ContFramePool::release_frames((*pt_vrt_adr >> 12));
    *pt_vrt_adr = 0 | 2;
  }else{
    assert(false);
  }
  write_cr3((unsigned long) page_directory);
  Console::puts("freed page\n");
}
