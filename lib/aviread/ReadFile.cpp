#include "ReadStreamV.h"
#include "ReadStreamA.h"
#include "avm_creators.h"
#include "avm_output.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

AVM_BEGIN_NAMESPACE;

IReadFile::~IReadFile() {}

class ReadFile: public IReadFile
{
    IMediaReadHandler* m_pHandler;
    avm::vector<ReadStream*> m_vstreams;
    avm::vector<ReadStream*> m_astreams;
public:
    ReadFile(const char* name, unsigned int flags) : m_pHandler(0)
    {
	if (!name || !strlen(name))
	    return;

	AVM_WRITE("reader", "Checking: %s\n", name);
	const char* ff = getenv("AVIPLAY_FFMPEG");
	if (ff)
	    m_pHandler = CreateFFReadHandler(name);

	if (!m_pHandler && !strstr(name, "://"))
	    m_pHandler = CreateAviReadHandler(name, flags);

	if (!m_pHandler)
	    m_pHandler = CreateAsfReadHandler(name);

	if (!m_pHandler && !ff)
	    m_pHandler = CreateFFReadHandler(name);

	if (!m_pHandler)
	    AVM_WRITE("reader", "Can't open stream\n");//never get here
    }
    ~ReadFile()
    {
	for (unsigned i = 0; i < m_vstreams.size(); i++)
	    delete m_vstreams[i];

	for (unsigned i = 0; i < m_astreams.size(); i++)
	    delete m_astreams[i];

	delete m_pHandler;
    }
    virtual uint_t StreamCount()
    {
	return VideoStreamCount() + AudioStreamCount();
    }
    virtual uint_t AudioStreamCount()
    {
	if (m_pHandler && !m_astreams.size())
	{
	    if (!m_vstreams.size() && !IsValid())
		return 0;
	    m_astreams.resize(m_pHandler->GetStreamCount(IStream::Audio));
	    for (uint_t i = 0; i < m_astreams.size(); i++)
                m_astreams[i] = 0;
	}
	return m_astreams.size();
    }
    virtual uint_t VideoStreamCount()
    {
	if (m_pHandler && !m_vstreams.size())
	{
	    if (!m_astreams.size() && !IsValid())
		return 0;
	    m_vstreams.resize(m_pHandler->GetStreamCount(IStream::Video));
	    for (uint_t i = 0; i < m_vstreams.size(); i++)
                m_vstreams[i] = 0;
	}
	return m_vstreams.size();
    }
    virtual uint_t GetHeader(void* header, uint_t size)
    {
	return m_pHandler ? m_pHandler->GetHeader(header, size) : 0;
    }
    virtual IReadStream* GetStream(uint_t id, IStream::StreamType type)
    {
	switch (type)
	{
	case IStream::Audio:
	    if (id < AudioStreamCount())
	    {
		if (!m_astreams[id])
		{
		    IMediaReadStream* r = m_pHandler->GetStream(id, type);
		    if (!r)
                        return 0;
		    m_astreams[id] = new ReadStreamA(r);
		}
                return m_astreams[id];
	    }
            break;
	case IStream::Video:
	    if (id < VideoStreamCount())
	    {
		if (!m_vstreams[id])
		{
		    IMediaReadStream* r = m_pHandler->GetStream(id, type);
		    if (!r)
                        return 0;
		    m_vstreams[id] = new ReadStreamV(r);
		}
                return m_vstreams[id];
	    }
            break;
	default:
            break;
	}
	return 0;
    }
    virtual bool GetURLs(avm::vector<avm::string>& urls)
    {
	return m_pHandler ? m_pHandler->GetURLs(urls) : false;
    }
    virtual bool IsOpened() { return m_pHandler ? m_pHandler->IsOpened() : true; }
    virtual bool IsValid() { return m_pHandler ? m_pHandler->IsValid() : false; }
    virtual bool IsRedirector() { return m_pHandler ? m_pHandler->IsRedirector() : false; }
};


IReadFile* CreateReadFile(const char* name, unsigned int flags)
{
    ReadFile* r = new ReadFile(name, flags);
    return r;
}

AVM_END_NAMESPACE;
