#ifndef CCAP_H
#define CCAP_H

#include <avm_stl.h>
#include <avm_locker.h>

class v4lxif;

class ClosedCaption
{
public:
    typedef void (*callbackproc) (void*, const char*);

    //ClosedCaption(const char* pcDevName="/dev/vbi");
    ClosedCaption(v4lxif* pv4l);
    ~ClosedCaption();

    const char* getBuffer() { return storage; }
    void add_callback(callbackproc p, void* arg);
    void remove_callback(callbackproc p, void* arg);

    void addref() { m_iRefcnt++; }
    void release() { m_iRefcnt--; if (m_iRefcnt==0) delete this; }
    void lock() { m_mutex.Lock(); }
    void unlock() { m_mutex.Unlock(); }
private:
    int CCdecode(int data);
    int decode(unsigned char *vbiline);
    void* mainloop();
    static void* threadstarter(void*);

    int m_iRefcnt;
    struct callback_info
    {
	callbackproc proc;
	void* arg;
	void exec(const char* p) { return proc(arg, p); }
    };
    avm::vector<callback_info> m_callbacks;


    // Member variables
    v4lxif* m_pV4l;
    avm::PthreadTask* m_pThread;
    avm::PthreadMutex m_mutex;
    int m_iDev;
    int m_iQuit;
    int	lastcode;
    int	ccmode;		//cc1 or cc2
    char plain;
    char ccbuf[3][256];		//cc is 32 columns per row, this allows for extra characters
    char storage[256];

    // Global constants
    static const char* const ratings[];
    static const int rowdata[];
    static const char* const specialchar[];
    static const char* const modes[];
};

#endif // CCAP_H
