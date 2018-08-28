#ifndef AVIFILE_MODULE_H
#define AVIFILE_MODULE_H

#include "infotypes.h"
#include "videoencoder.h"

#include "loader.h"

AVM_BEGIN_NAMESPACE;


struct DRVR
{
    unsigned int	uDriverSignature;
    int			hDriverModule;
    DRIVERPROC		DriverProc;
    unsigned long	dwDriverID;
};

class Module;

class VideoCodecControl
{
friend class Module;
    avm::vector<Module*> _modules;
public:
    VideoCodecControl() {}
    Module* Create(const CodecInfo& info);
    ~VideoCodecControl();
protected:
    void Erase(Module*);
};

class Module
{
public:
    enum Mode {Compress, Decompress};
    Module(const char* name, VideoCodecControl& parent); //loads module
    ~Module();
    const char* GetName() const { return (const char*)m_pName; }
    HIC CreateHandle(unsigned int compressor, Mode mode);
    int CloseHandle(HIC);
    int GetLibHandle() { return m_pHandle; }
    void ForgetParent() { forgotten = 1; }
    int init();
protected:
    VideoCodecControl& _parent;
    avm::string m_pName;
    DRVR driver;
    int forgotten;
    int _refcount;
    int m_pHandle;
};

extern VideoCodecControl control;

AVM_END_NAMESPACE;

#endif // AVIFILE_MODULE_H
