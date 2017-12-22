/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
extern Scheduler * SYSTEM_SCHEDULER;
BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
  diskQHead = NULL;
  diskQTail = NULL;
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
//  SimpleDisk::read(_block_no, _buf);
//  return;
  diskNode* newNode = new diskNode;
  newNode->thread = Thread::CurrentThread();
  newNode->next = NULL;
  if(diskQHead == NULL){
    diskQHead = newNode; 
    diskQTail = newNode; 
  }else{
    diskQTail->next = newNode;
  }
  //QUEUE add an element
  while( diskQHead->thread != Thread::CurrentThread()){
    SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
  }
  issue_operation(READ, _block_no);
  while(!SimpleDisk::is_ready()){
    SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
  }
  /* read data from port */
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = Machine::inportw(0x1F0);
    _buf[i*2]   = (unsigned char)tmpw;
    _buf[i*2+1] = (unsigned char)(tmpw >> 8);
  }
  diskQHead = diskQHead->next;
  delete newNode;
  //QUEUE delete element
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
//  SimpleDisk::write(_block_no, _buf);
//  return; 
  diskNode* newNode = new diskNode;
  newNode->thread = Thread::CurrentThread();
  newNode->next = NULL;
  if(diskQHead == NULL){
    diskQHead = newNode; 
    diskQTail = newNode; 
  }else{
    diskQTail->next = newNode;
  }
  //QUEUE add an element
  while( diskQHead->thread != Thread::CurrentThread()){
    SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
  }
  issue_operation(WRITE, _block_no);
  while(!SimpleDisk::is_ready()){
    SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
  }
  /* write data to port */
  int i; 
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    Machine::outportw(0x1F0, tmpw);
  }
  diskQHead = diskQHead->next;
  delete newNode;
  //QUEUE delete element
}
