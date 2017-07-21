/*
 File: scheduler.C
 
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

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "simple_timer.H"
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
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  rdyQTail = NULL;
  rdyQHead = NULL;
    
  //assert(false);
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  Machine::disable_interrupts();
  if(rdyQHead == NULL) assert(true);
  myQ* nextEle = rdyQHead;
  Thread *nextThread = nextEle->thread;
  if(rdyQHead == rdyQTail) rdyQTail = NULL;
  rdyQHead = rdyQHead->next;
  delete nextEle;
  Thread::dispatch_to(nextThread);
  Machine::enable_interrupts();
//  assert(false);
}

void Scheduler::resume(Thread * _thread) {
  Machine::disable_interrupts();
  myQ* newThread = new myQ;
  newThread->thread = _thread;
  newThread->next = NULL;
  if(rdyQHead == NULL){
    rdyQHead = newThread;
    rdyQTail = newThread;
  }else{
    rdyQTail->next = newThread;
    rdyQTail = rdyQTail->next;
  }
  Machine::enable_interrupts();
//  assert(false);
}

void Scheduler::add(Thread * _thread) {
  resume(_thread);  
//  assert(false);
}

void Scheduler::terminate(Thread * _thread) { 
  delete _thread;
  yield();
//  assert(false);
}


RRScheduler::RRScheduler(unsigned _eoq){
  rdyQTail = NULL;
  rdyQHead = NULL;
  eoq = _eoq;
  SimpleTimer *timer = new SimpleTimer(1000/eoq); /* timer ticks every 50ms. */
  InterruptHandler::register_handler(0, timer);
  /* The Timer is implemented as an interrupt handler. */
}

void RRScheduler::yield() {
//  Machine::disable_interrupts();
  if(rdyQHead == NULL) assert(true);
  myQ* nextEle = rdyQHead;
  Thread *nextThread = nextEle->thread;
  if(rdyQHead == rdyQTail) rdyQTail = NULL;
  rdyQHead = rdyQHead->next;
  delete nextEle;
  Thread::dispatch_to(nextThread);
  Machine::enable_interrupts();
//  assert(false);
}

void RRScheduler::resume(Thread * _thread) {
//  Machine::disable_interrupts();
  myQ* newThread = new myQ;
  newThread->thread = _thread;
  newThread->next = NULL;
  if(rdyQHead == NULL){
    rdyQHead = newThread;
    rdyQTail = newThread;
  }else{
    rdyQTail->next = newThread;
    rdyQTail = rdyQTail->next;
  }
//  Machine::enable_interrupts();
//  assert(false);
}

void RRScheduler::add(Thread * _thread) {
  Machine::disable_interrupts();
  myQ* newThread = new myQ;
  newThread->thread = _thread;
  newThread->next = NULL;
  if(rdyQHead == NULL){
    rdyQHead = newThread;
    rdyQTail = newThread;
  }else{
    rdyQTail->next = newThread;
    rdyQTail = rdyQTail->next;
  }
  Machine::enable_interrupts();
//  assert(false);
}

void RRScheduler::terminate(Thread * _thread) { 
  delete _thread;
  yield();
//  assert(false);
}

