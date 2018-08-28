#ifndef AVIFILE_ASXREADER_H
#define AVIFILE_ASXREADER_H

#include "avm_stl.h"

AVM_BEGIN_NAMESPACE;

class ASX_Reader
{
public:
    ASX_Reader() {} // for redirector's default constructor
    ASX_Reader(const avm::string& server, const avm::string& file)
	: m_Server(server), m_Filename(file) {}
    bool create(const char* data, uint_t size);
    bool getURLs(avm::vector<avm::string>& urls) const;
private:
    bool addURL(const char* url);
    avm::string m_Server;
    avm::string m_Filename;
    avm::vector<avm::string> m_Urls;
};

AVM_END_NAMESPACE;

#endif // AVIFILE_ASXREADER_H
