// synch.cc 
//  Routines for synchronizing threads.  Three kinds of
//  synchronization routines are defined here: semaphores, locks 
//      and condition variables (the implementation of the last two
//  are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
//  Initialize a semaphore, so that it can be used for synchronization.
//
//  "debugName" is an arbitrary name, useful for debugging.
//  "initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
//  De-allocate semaphore, when no longer needed.  Assume no one
//  is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
//  Wait until semaphore value > 0, then decrement.  Checking the
//  value and decrementing must be done atomically, so we
//  need to disable interrupts before checking the value.
//
//  Note that Thread::Sleep assumes that interrupts are disabled
//  when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts

    while (value == 0) {            // semaphore not available
        queue->Append((void *)currentThread);   // so go to sleep
        currentThread->Sleep();
    }
    value--;            // semaphore available,
                        // consume its value
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
//  Increment semaphore value, waking up a waiter if necessary.
//  As with P(), this operation must be atomic, so we need to disable
//  interrupts.  Scheduler::ReadyToRun() assumes that threads
//  are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)    // make thread ready, consuming the V immediately
    scheduler->ReadyToRun(thread);
    value++;

    (void) interrupt->SetLevel(oldLevel);
}


// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!

//==================Lock==================================================
Lock::Lock(char* debugName) {
    name=debugName;
    semLock=new Semaphore(debugName,1);
    heldByThread=NULL;
}

Lock::~Lock() {
    delete semLock;
}

void Lock::Acquire() {
    ASSERT(!isHeldByCurrentThread());
    DEBUG('l',"thread %s try to acquire lock\n",currentThread->getName());
    semLock->P();
    heldByThread=currentThread;
    DEBUG('l',"\033[1;33;40mlock Acquired by thread: %s\033[m\n",currentThread->getName());
}

void Lock::Release() {
    ASSERT(isHeldByCurrentThread());
    heldByThread=NULL;
    DEBUG('l',"\033[1;33;40mlock Released by thread: %s\033[m\n\n",currentThread->getName());
    if(semLock->value==0)
        semLock->V();
    // if(semLock->value>1)semLock->value=1;

}

bool Lock::isHeldByCurrentThread(){
    return heldByThread==currentThread;
}


//=================condition===================================================

Condition::Condition(char* debugName) {
    name=debugName;
    semCond=new Semaphore(debugName,0);
}

Condition::~Condition() {
    delete semCond;
}

void Condition::Wait(Lock* conditionLock) {
    ASSERT(conditionLock->isHeldByCurrentThread());
    DEBUG('c',"\033[1;34;40mthread %s Wait\033[m\n",currentThread->getName());
    conditionLock->Release();
    semCond->P();
    conditionLock->Acquire();
}

void Condition::Signal(Lock* conditionLock) {
    ASSERT(conditionLock->isHeldByCurrentThread());
    if(!semCond->queue->IsEmpty())
        semCond->V();
    if(semCond->value>1)
        semCond->value=1;
    DEBUG('c',"\033[1;34;40mthread %s Signal\033[m\n",currentThread->getName());
}

void Condition::Broadcast(Lock* conditionLock) {
    ASSERT(conditionLock->isHeldByCurrentThread());
    int i=0;
    while(!semCond->queue->IsEmpty()){
        semCond->V();
        ++i;
    }
    if(semCond->value>1)
        semCond->value=1;
    DEBUG('b',"***** value: %d, len(queue): %d\n",semCond->value,i);
    DEBUG('c',"\033[1;34;40mthread %s Broadcast\033[m\n",currentThread->getName());
}
