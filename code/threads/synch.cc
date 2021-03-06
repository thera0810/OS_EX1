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
    value--;                    // semaphore available, 
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



// ====================   LOCK   ==============================================


// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {
    name=debugName;
    mutex=1;
    queue = new List;
    heldByThread=NULL;
}

Lock::~Lock() {
    delete queue;
}

void Lock::Acquire() {
    // avoid to acquire the same lock again
    ASSERT(!isHeldByCurrentThread());
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    DEBUG('l',"thread %s try to acquire lock\n",currentThread->getName());
    while(mutex==0){
        //can not enable int here because---Release can check the queue and not wake up thread.
        queue->Append((void *)currentThread);   // so go to sleep
        //can not enable int here because---Misses wakeup and still holds lock (deadlock!)
        DEBUG('l',"thread %s try to acquire lock, but failed\n",currentThread->getName());
        currentThread->Sleep();
    }
    mutex=0;
    DEBUG('l',"\033[1;33;40mlock Acquired by thread: %s\033[m\n",currentThread->getName());
    heldByThread=currentThread;
    (void) interrupt->SetLevel(oldLevel);
}

void Lock::Release() {
    ASSERT(isHeldByCurrentThread());
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    Thread *thread = (Thread *)queue->Remove();
    mutex=1;
    heldByThread=NULL;
    DEBUG('l',"\033[1;33;40mlock Released by thread: %s\033[m\n\n",currentThread->getName());
    if (thread != NULL){    // make thread ready, consuming the V immediately
        scheduler->ReadyToRun(thread);
    }
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

bool Lock::isHeldByCurrentThread(){
    //atomicity?
    return heldByThread==currentThread;
}





//=================   CONDITION   =====================================================

Condition::Condition(char* debugName) {
    firstLock=NULL;
    name=debugName;
    queue=new List;
}

Condition::~Condition() {
    delete queue;
    delete firstLock;
}

void Condition::Wait(Lock* conditionLock) {
    if (firstLock==NULL)
    {
        firstLock=conditionLock;
    }
    ASSERT(firstLock==conditionLock);
    ASSERT(conditionLock->isHeldByCurrentThread());
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    DEBUG('c',"\033[1;34;40mthread %s Wait\033[m\n",currentThread->getName());
    queue->Append((void *)currentThread);   // so go to sleep
    conditionLock->Release();
    currentThread->Sleep();
    conditionLock->Acquire();
    
    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Signal(Lock* conditionLock) {
    if (firstLock==NULL)
    {
        firstLock=conditionLock;
    }
    ASSERT(firstLock==conditionLock);
    ASSERT(conditionLock->isHeldByCurrentThread());
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    
    thread = (Thread *)queue->Remove();
    if(thread!=NULL)
        scheduler->ReadyToRun(thread);
    DEBUG('c',"\033[1;34;40mthread %s Signal\033[m\n",currentThread->getName());
    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast(Lock* conditionLock) {
    if (firstLock==NULL)
    {
        firstLock=conditionLock;
    }
    ASSERT(firstLock==conditionLock);
    ASSERT(conditionLock->isHeldByCurrentThread());
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    while(!queue->IsEmpty()){
        thread = (Thread *)queue->Remove();
        scheduler->ReadyToRun(thread);
    }
    DEBUG('c',"\033[1;34;40mthread %s Broadcast\033[m\n",currentThread->getName());
    (void) interrupt->SetLevel(oldLevel);
}
