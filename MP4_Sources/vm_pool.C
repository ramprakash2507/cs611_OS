/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"
/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    //assert(false);
    base_address = _base_address;
    size = _size;
    framePool = _frame_pool;
    page_table = _page_table;   
    page_table->register_pool(this);
    memList * vmTable = (memList *)base_address;
    vmTable->start_page = (base_address >> 12); 
    vmTable->size = 1; 
    for(int i=1;i<512;i++){
        vmTable++;
     //   Console::puts("Curr Addr");Console::puti((unsigned long)vmTable);Console::puts("\n");
        vmTable->start_page = -1;
        vmTable->size = 0;
    }
    //assert(false);
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    //assert(false);
    unsigned long pages = (_size & 0x00000fff)?((_size>>12) + 1):(_size>>12);
    memList * vmTable = (memList *)base_address; 
    memList * vmTable_prev = (memList *)base_address; 
    int i=1;
    while(i<512){
        vmTable++;
        if(vmTable->size == 0){
            vmTable_prev = vmTable - 1;
            if((vmTable->start_page - vmTable_prev->start_page) > (pages + vmTable_prev->size)){
                break;
            }
        }
        i++;
    }
    if(i == 512) assert(false);
    vmTable->start_page=vmTable_prev->start_page + vmTable_prev->size;
    vmTable->size = pages;
    Console::puts("Allocated region of memory.\n");
    return (vmTable->start_page << 12);
}

void VMPool::release(unsigned long _start_address) {
    //assert(false);
    unsigned long page = _start_address >> 12;
    memList * vmTable = (memList *)base_address; 
    memList * vmTable_next = (memList *)base_address; 
    int i=1;
    while(i<512){
        vmTable++;
        if(vmTable->start_page == page){
            vmTable_next = vmTable + 1;
            //vmTable->start_page = vmTable_next->start_page;
            //vmTable->size = 0;
            break;
        }
        i++;
    }
    if(i == 512) assert(false);
    for(int j = 0; j < vmTable->size; j++){
       page_table->free_page(vmTable->start_page + j);
    }
    vmTable->start_page = (i == 511)? -1 : vmTable_next->start_page;
    vmTable->size = 0;
    unsigned long strt_page = vmTable->start_page; 
    vmTable--;
    while((unsigned long)vmTable > base_address && vmTable->size == 0){
       vmTable->start_page = strt_page;
       vmTable--; 
    }
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    unsigned long page = _address >> 12;
    if(base_address > _address || _address >= base_address + size) return false;
    if(page == (base_address >> 12)) return true;
    int i = 0;
    while(i<512){
        memList * vmTable = (memList *)(base_address) + i; 
        if(vmTable->start_page <= page && page < (vmTable->start_page + vmTable->size)) break;
        i++;
    }
    Console::puts("Checked whether address is part of an allocated region.\n");
    return (i !=512);
   // assert(false);
}

