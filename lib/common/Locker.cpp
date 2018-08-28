#include "avm_locker.h"

// to get recursive mutexes
// #define __USE_UNIX98

#include <sys/time.h>
#include <errno.h>//EDEADLK
#include <stdio.h>
#include <pthread.h>

// doesn't exists  on BSD systems ??  #include <semaphore.h>

AVM_BEGIN_NAMESPACE;

PthreadMutex::PthreadMutex( /*Attr mattr */ )
{
    m_pMutex = new pthread_mutex_t;
#if 0
    pthread_mutexattr_t pma;
    pthread_mutexattr_init(&pma);

    if (mattr == RECURSIVE)
        pthread_mutexattr_settype(&pma, PTHREAD_MUTEX_RECURSIVE_NP);

    pthread_mutex_init((pthread_mutex_t*) m_pMutex, &pma);
#endif
    pthread_mutex_init((pthread_mutex_t*) m_pMutex, NULL);
}

PthreadMutex::~PthreadMutex()
{
    pthread_mutex_destroy((pthread_mutex_t*) m_pMutex);
    delete (pthread_mutex_t*) m_pMutex;
}

int PthreadMutex::Lock()
{
    return pthread_mutex_lock((pthread_mutex_t*) m_pMutex);
}

int PthreadMutex::TryLock()
{
    int r = pthread_mutex_trylock((pthread_mutex_t*) m_pMutex);
    return (r == EBUSY) ? -1 : 0;
}

int PthreadMutex::Unlock()
{
    return pthread_mutex_unlock((pthread_mutex_t*) m_pMutex);
}

PthreadCond::PthreadCond()
{
    m_pCond = new pthread_cond_t;
    pthread_cond_init((pthread_cond_t*) m_pCond, NULL);
}

PthreadCond::~PthreadCond()
{
    pthread_cond_destroy((pthread_cond_t*) m_pCond);
    delete (pthread_cond_t*) m_pCond;
}

int PthreadCond::Wait(PthreadMutex& m, float waitTime)
{
    if (waitTime >= 0.0)
    {
    	struct timespec timeout;
	struct timeval now;
	gettimeofday(&now, 0);


	timeout.tv_sec = now.tv_sec + (int)waitTime;
	waitTime -= (int)waitTime;
	timeout.tv_nsec = (now.tv_usec + (int)(waitTime * 1000000)) * 1000;

	if (timeout.tv_nsec >= 1000000000)
	{
	    timeout.tv_nsec -= 1000000000;
            timeout.tv_sec++;
	}
	// time limited conditional waiting here!
	// sometimes decoder thread could be stoped
	// and we would have wait here forever
	int r = pthread_cond_timedwait((pthread_cond_t*) m_pCond,
				       (pthread_mutex_t*) m.m_pMutex, &timeout);
	if (r < 0)
	    perror("PthreadCond::Wait()");
        return r;
    }

    return pthread_cond_wait((pthread_cond_t*) m_pCond,
			     (pthread_mutex_t*) m.m_pMutex);
}

int PthreadCond::Broadcast()
{
    return pthread_cond_broadcast((pthread_cond_t*) m_pCond);
}


PthreadTask::PthreadTask(void* attr, void* (*start_routine)(void *), void* arg)
{
    m_pTask = new pthread_t;
    if (pthread_create((pthread_t*) m_pTask,
		       (pthread_attr_t*) attr, start_routine, arg))
	perror("PthreadTask()");
}

PthreadTask::~PthreadTask()
{
    int result = pthread_join(* ((pthread_t*)m_pTask), 0);
    delete (pthread_t*) m_pTask;
    // throw exception later
    if (result == EDEADLK)
    {
	perror("~PthreadTask()");
	pthread_exit(NULL);
    }
}

Locker::Locker(PthreadMutex& mutex) : m_pMutex(mutex.m_pMutex)
{
    pthread_mutex_lock((pthread_mutex_t*) m_pMutex);
}

Locker::~Locker()
{
    pthread_mutex_unlock((pthread_mutex_t*) m_pMutex);
}

AVM_END_NAMESPACE;
