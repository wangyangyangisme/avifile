#ifndef ASFREDIRECTINPUTSTREAM_H
#define ASFREDIRECTINPUTSTREAM_H

#include "AsxReader.h"
#include "AsfInputStream.h"

AVM_BEGIN_NAMESPACE;

class AsfRedirectInputStream: public AsfInputStream
{
    avm::vector<char> m_Buffer;
    ASX_Reader m_Reader;
public:
    AsfRedirectInputStream() :m_Buffer(16384) {}
    virtual bool getURLs(avm::vector<avm::string>& urls) { return m_Reader.getURLs(urls); }
    virtual bool isRedirector() { return true; }

    int init(const char* pszFile);
};

AVM_END_NAMESPACE;

#endif // ASFREDIRECTINPUTSTREAM_H
