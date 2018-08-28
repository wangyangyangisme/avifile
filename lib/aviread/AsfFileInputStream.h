#ifndef AVIFILE_ASFFILEINPUTSTREAM_H
#define AVIFILE_ASFFILEINPUTSTREAM_H

#include "AsfInputStream.h"
#include "avm_locker.h"

/**
    Relative complexity of this model serves the following purposes:
    1) Local and remote ASF files should be seekable from player
      ( allow to simultaneously re-seek all streams to the same position ).
    2) Data downloaded from network stream should be cached ( downloaded
    before it's requested ) and discarded when it's not inter needed
    3) Local ASF files should allow independent seeking in streams.
    4) Local ASF files should allow their conversion to AVI to as much
    extent as it is possible.
**/

AVM_BEGIN_NAMESPACE;

class FileIterator;

class AsfFileInputStream: public AsfInputStream
{
friend class FileIterator;
public:
    AsfFileInputStream();
    virtual ~AsfFileInputStream();
    virtual AsfIterator* getIterator(uint_t id);
    int init(const char* pszFile);
protected:
    void createSeekData();
    int64_t m_lDataOffset;
    int m_iFd;
    PthreadMutex m_Mutex;
    avm::vector<AsfStreamSeekInfo*> m_SeekInfo;
};

AVM_END_NAMESPACE;

#endif // AVIFILE_ASFFILEINPUTSTREAM_H
