#ifndef FILEBUFFER_H
#define FILEBUFFER_H

#include "avm_default.h"

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include "avm_except.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h> // memcpy

AVM_BEGIN_NAMESPACE;

#define __MODULE__ "File buffer"

class FileBuffer
{
    int _fd;
    char* _buffer;
    uint_t _bufsize;
    uint_t _intlimit;
    uint_t _used;
public:
    FileBuffer(const char* fn, int mode = O_WRONLY | O_TRUNC | O_CREAT,
	       int mask = 00644, uint_t bufsize = 131072)
    {
	if (bufsize < 0)
	    throw FATAL("Invalid argument");
	//printf("Opening %s\n", fn);
	_fd = open(fn, mode, mask);
	if (_fd < 0)
	    throw FATAL("Could not open file");
	_buffer = new char[bufsize];
	_bufsize = bufsize;
	_intlimit = bufsize - 4;
	_used = 0;
    }
    ~FileBuffer()
    {
	sync();
	close(_fd);
	delete[] _buffer;
    }
    void sync()
    {
	::write(_fd, _buffer, _used);
	_used = 0;
    }
    off_t lseek(off_t offset, int whence)
    {
//	return ::lseek(_fd, offset, whence);

	if (offset==0 && whence==SEEK_CUR)
	    return ::lseek(_fd, 0, SEEK_CUR)+_used;
	sync();
	return ::lseek(_fd, offset, whence);

    }
    ssize_t write(const void* buffer, uint_t count)
    {
//	return ::write(_fd, buffer, count);
	ssize_t written=0;
	while (count)
	{
	    uint_t size = (_bufsize-_used);
	    if (count < size)
		size = count;
	    memcpy(_buffer+_used, buffer, size);
	    count-=size;
	    _used+=size;
	    written+=size;
	    buffer=(const char*)buffer+size;
	    if (_used==_bufsize)
		sync();
	    if ((_used==0) && (count>_bufsize))
	    {
                // fixme - check returned value!!!
		::write(_fd, buffer, count);
	        written += count;
	        break;
	    }	
	}
	return written;
    }
    void write_le32(int value)
    {
	//::write(_fd, &value, 4);
	if (_used >= _intlimit)
	    sync();
	_buffer[_used++] = value & 0xff;
	value >>= 8;
	_buffer[_used++] = value & 0xff;
	value >>= 8;
	_buffer[_used++] = value & 0xff;
	value >>= 8;
	_buffer[_used++] = value;
    }
    void write_be32(int value)
    {
	//::write(_fd, &value, 4);
	if (_used >= _intlimit)
	    sync();
	_buffer[_used++] = (value >> 24) & 0xff;
	_buffer[_used++] = (value >> 16) & 0xff;
	_buffer[_used++] = (value >> 8) & 0xff;
	_buffer[_used++] = value;
    }
};

#undef __MODULE__

AVM_END_NAMESPACE;

#endif // FILEBUFFER_H
