#include "Module.h"

extern "C" {
#include "wine/winreg.h"
#include "driver.h"
}

#include "avm_output.h"
#include <stdio.h>
#include <string.h>

AVM_BEGIN_NAMESPACE;

VideoCodecControl control;

VideoCodecControl::~VideoCodecControl()
{
    while (_modules.size() > 0)
    {
	Module* m = _modules.back();
        _modules.pop_back();
	m->ForgetParent();
	// at this time player using this codec has to be stopped
	// currently this is partial responsibility of user task
        // FIXME !!! - destroy codec thread if necessary
    }
}

Module* VideoCodecControl::Create(const CodecInfo& info)
{
    //printf("VideoCodecControl::Create %s\n", info.dll.c_str());
    for (unsigned i = 0; i < _modules.size(); i++)
    {
	if (strcmp(_modules[i]->GetName(), (const char*) info.dll) == 0)
	{
	    //printf("VideoCodecControl::Create cache %d\n", i);
	    return _modules[i];
	}
    }
    Module* module = new Module((const char*)info.dll, *this);
    if (module->init() < 0)
    {
        delete module;
	return 0;
    }

    _modules.push_back(module);
    return module;
}

void VideoCodecControl::Erase(Module* mod)
{
    //printf("VideoCodecControl::Erase module %p\n", mod);
    for (unsigned i = 0; i < _modules.size(); i++)
	if (_modules[i] == mod)
	{
            // remove this member (swap with last one)
	    Module* m = _modules.back();
	    _modules.pop_back();
	    if (_modules.size() > i)
		_modules[i] = m;

	    //printf("VideoCodecControl::Erase success - new _modules size %d  %p\n", _modules.size(), m);
	    //if (_modules.size() > 0) printf("VideoCodecControl::Erase last %s\n", _modules[_modules.size() - 1]->GetName());
	    break;
	}
}

Module::Module(const char* name, VideoCodecControl& parent)
    :_parent(parent), m_pName(name), forgotten(0), _refcount(0)
{
}

int Module::init()
{
    m_pHandle = LoadLibraryA(m_pName);
    if (!m_pHandle)
    {
	AVM_WRITE("Win32 plugin", "Could not load Win32 dll library: %s\n", m_pName.c_str());
        return -1;
    }

    CodecAlloc();

    driver.uDriverSignature = 0;
    driver.hDriverModule = m_pHandle;
    driver.dwDriverID = 0;
    driver.DriverProc = (DRIVERPROC) GetProcAddress(m_pHandle, "DriverProc");

    if (!driver.DriverProc)
    {
	AVM_WRITE("Win32 plugin", "Not a valid Win32 dll library: %s\n", m_pName.c_str());
	return -1;
    }

    ICSendMessage((int)&driver, DRV_LOAD, 0, 0);
    ICSendMessage((int)&driver, DRV_ENABLE, 0, 0);

    AVM_WRITE("Win32 plugin", "Using Win32 dll library: %s\n", m_pName.c_str());
    return 0;
}

Module::~Module()
{
    if (m_pHandle)
    {
	if (driver.DriverProc)
	    ICSendMessage((int)&driver, DRV_FREE, 0, 0); // 0 drivreid
	FreeLibrary(m_pHandle);
	CodecRelease();
    }

    if (!forgotten)
	_parent.Erase(this);
}

HIC Module::CreateHandle(unsigned int compressor, Mode mode)
{
    ICOPEN icopen;
    icopen.fccType    = mmioFOURCC('v', 'i', 'd', 'c');
    icopen.fccHandler = compressor;
    icopen.dwSize     = sizeof(ICOPEN);
    icopen.dwFlags    = (mode == Compress) ? ICMODE_COMPRESS : ICMODE_DECOMPRESS;

    driver.dwDriverID = ++_refcount;
    DRVR* hDriver = new DRVR(driver);
    //printf("Creating new handle for %s _refcount %d, driver %d\n", Name(), _refcount,(int)hDriver);
    //printf("fcc: 0x%lx,  handler: 0x%lx  (%.4s)\n", icopen.fccType, icopen.fccHandler, (char*) &icopen.fccHandler);

    hDriver->dwDriverID = ICSendMessage((int)hDriver, DRV_OPEN, 0, (int) &icopen);
    if (!hDriver->dwDriverID)
    {
	AVM_WRITE("Win32 plugin", "WARNING DRV_OPEN failed (0x%lx)\n", icopen.fccHandler);
        return 0;
    }
    //else printf("New ID: %ld\n", hDriver->dwDriverID);
    //printf("CREATE HANDLE %p   %d\n", hDriver, _refcount);
    return (HIC) hDriver;
}

int Module::CloseHandle(HIC handle)
{
    DRVR* hDriver = (DRVR*) handle;
    //printf("DELETE HANDLE %p   %d\n", hDriver, _refcount);

    if (hDriver)
	ICSendMessage((int)hDriver, DRV_CLOSE, 0, 0);

    if (--_refcount == 0)
	delete this;
    delete hDriver;

    return 0;
}

AVM_END_NAMESPACE;
