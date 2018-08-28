#include "AsfRedirectInputStream.h"
#include "avm_output.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

AVM_BEGIN_NAMESPACE;

int AsfRedirectInputStream::init(const char* pszFile)
{
    int fd = open(pszFile, O_RDONLY);
    if (fd < 0)
    {
	AVM_WRITE("ASX reader", "Could not open file!\n");
        return -1;
    }
    int r = read(fd, &m_Buffer[0], m_Buffer.size());
    close(fd);

    if (r > 0 && m_Reader.create(&m_Buffer[0], m_Buffer.size()))
	return 0;

    AVM_WRITE("ASX reader", "Not a redirector!\n");
    return -1;
}

AVM_END_NAMESPACE;
