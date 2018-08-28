#ifndef AVM_LOCKER_H
#define AVM_LOCKER_H

#include "avm_default.h"

AVM_BEGIN_NAMESPACE;

// do not include pthread.h nor semaphore.h here
// hidden in the implementation

class PthreadCond;
class Locker;

/**
 * class used to hide usage of thread
 *
 * it might be implemented diferently for various platforms
 */
class PthreadMutex
{
    void* m_pMutex;
friend class PthreadCond;
friend class Locker;
    /// \internal disabled
    PthreadMutex(const PthreadMutex&) :m_pMutex(0) {}
    /// \internal disabled
    PthreadMutex& operator=(const PthreadMutex&) { return *this; }
public:
    // most probably unportable
    // enum Attr { FAST, RECURSIVE };
    PthreadMutex( /* Attr = FAST */ );
    ~PthreadMutex();
    /// Lock mutex
    int Lock();
    /// TryLock mutex
    int TryLock();
    /// Unlock mutex
    int Unlock();
};

/**
 *
 */
class PthreadCond
{
    void* m_pCond;
    /// \internal disabled
    PthreadCond(const PthreadCond&) :m_pCond(0) {}
    /// \internal disabled
    PthreadCond& operator=(const PthreadCond&) { return *this; }
public:
    PthreadCond();
    ~PthreadCond();
    int Wait(PthreadMutex& m, float waitTime = -1.0);
    int Broadcast();
};

/**
 * Creates a new thread of control that executes concurrently with
 * the calling thread.
 */
class PthreadTask
{
    void* m_pTask;
    /// \internal disabled
    PthreadTask(const PthreadTask&) :m_pTask(0) {}
    /// \internal disabled
    PthreadTask& operator=(const PthreadTask&) { return *this; }
public:
    PthreadTask(void* attr, void* (*start_routine)(void *), void* arg);
    ~PthreadTask();
};

/**
 * Simple mutex locker which makes the mutex usage easier
 */
class Locker
{
    void* m_pMutex;
    /// \internal disabled
    Locker(const Locker&) :m_pMutex(0) {}
    /// \internal disabled
    Locker& operator=(const Locker&) { return *this; }
public:
    Locker(PthreadMutex& mutex);
    ~Locker();
};

AVM_END_NAMESPACE;

#ifdef AVM_COMPATIBLE
typedef avm::Locker Locker;
typedef avm::PthreadCond PthreadCond;
typedef avm::PthreadMutex PthreadMutex;
#endif

#endif // AVM_LOCKER_H
