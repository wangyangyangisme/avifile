#ifdef HAVE_CONFIG_H
#include <config.h>  // config.h by configure
#endif

#include "dsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/soundcard.h>
#include <sys/ioctl.h>

dsp::dsp()
    : buffer(0)
{
}

dsp::~dsp()
{
    close();
}

static void dump_buf_info(audio_buf_info& z)
{
    printf("%d/%d fragments available\n", z.fragments, z.fragstotal);
    printf("%d bytes each fragment\n", z.fragsize);
    printf("%d bytes available\n", z.bytes);
}

int dsp::open(int bits, int channels, int rate, const char* dev)
{
    int afmt, trigger;

    fd = ::open(dev, O_RDONLY);
    if (fd == -1)
    {
	fprintf(stderr, "open: %s: %s\n", dev, strerror(errno));
        return -1;
    }

    ioctl(fd, SNDCTL_DSP_RESET, 0);
    int frag = (8<<16)|(10); //8 buffers, 1024 bytes each
    ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag);
    /* format */
    switch (bits) {
    case 16:
	afmt = AFMT_S16_LE;
	ioctl(fd, SNDCTL_DSP_SETFMT, &afmt);
	if (afmt == AFMT_S16_LE)
	    break;
	/* fall back*/
	fprintf(stderr,"no 16 bit sound, trying 8 bit...\n");
	bits = 8;
    case 8:
	afmt = AFMT_U8;
	ioctl(fd, SNDCTL_DSP_SETFMT, &afmt);
	if (afmt != AFMT_U8) {
	    fprintf(stderr,"Oops: no 8 bit sound ?\n");
	    goto err;
	}
	break;
    default:
	fprintf(stderr,"%d bit sound not supported\n",
		bits);
	goto err;
    }


    ioctl(fd, SNDCTL_DSP_CHANNELS, &channels); /* channels */
    ioctl(fd, SNDCTL_DSP_SPEED, &rate); /* sample rate */

    if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE,  &blocksize) == -1)
	goto err;
    printf("blocksize: %d\n",blocksize);
    blocksize *= 4;
    buffer = (char*)malloc(blocksize);

    /* trigger record */
    trigger = ~PCM_ENABLE_INPUT;
    ioctl(fd, SNDCTL_DSP_SETTRIGGER, &trigger);
    trigger = PCM_ENABLE_INPUT;
    ioctl(fd, SNDCTL_DSP_SETTRIGGER, &trigger);

#if 0
    struct audio_buf_info z;
    ioctl(fd, SNDCTL_DSP_GETISPACE, &z);
    dump_buf_info(z);
#endif
    return fd;

err:
    return -1;
}

int dsp::close()
{
    if (fd != -1)
	::close(fd);

    if (buffer)
	free(buffer);
    buffer = 0;
    fd = -1;
    return 0;
}

int dsp::read(char* rbuffer, int bsize)
{
    int r = ::read(fd, rbuffer, bsize);
    if (r <= 0)
    {
	perror("read /dev/dsp");
	return r;
    }
    return r;
}

int dsp::synch()
{
    struct audio_buf_info z;
    ioctl(fd, SNDCTL_DSP_GETISPACE, &z);
    dump_buf_info(z);
    //	usleep(100000);
    //  ioctl(fd, SNDCTL_DSP_GETISPACE, &z);
    //	dump_buf_info(z);
    int bytes_read = 0;
    while (z.bytes)
    {
	int bytes_to_read = z.bytes > blocksize ? blocksize : z.bytes;
	bytes_to_read = ::read(fd, buffer, bytes_to_read);
	if (bytes_to_read < 0 && errno != EINTR)
	    break;
	//printf("Bread %d\n", bytes_to_read);
	bytes_read += bytes_to_read;
	z.bytes -= bytes_to_read;
    }
    printf("Flushed %d bytes of data from audio buffers\n", bytes_read);
    return 0;
}

int dsp::getSize()
{
    struct audio_buf_info z;
    ioctl(fd, SNDCTL_DSP_GETISPACE, &z);
    return z.bytes;
}
