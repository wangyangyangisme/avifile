/**
 *
 * Test sample for checking memory leaks in audio decoding
 *
 * User should have mpatrol library installed
 * (In case it's not available - modify Makefile.am apropriately)
 * It's good idea to use single threaded operation mode
 * so modify Cache.cpp to no use thread for precaching #define NOTHREADS
 *
 * to use this program with mpatrol:
 *    mpatrol --show-all --leak-table testaudio
 * with libefence:
 *    LD_PRELOAD=/usr/lib/libefence.so.0 testaudio
 *
 */

#include <avifile.h>
#include <avm_except.h>
#include <utils.h>

#ifdef USE_MPATROL
#include <mpatrol.h>
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

const char* filename = "/net/hdd2/movies/wm9a/world_192_mulitchannel.wma";
//const char* filename = "/net/hdd2/movies/testE/indy_500.wmv";
//const char* filename = "/net/hdd2/movies/test/wm8_vandread.wmv";
//const char* filename = "/net/hdd2/movies/test/Lucky.asf";
//const char* filename = "/net/hdd2/movies/test/04.asf";
//const char* filename = "/net/hdd2/movies/test/Gone.avi";
//const char* filename = "/net/hdd2/movies/test3/imaadcmp.avi";

int main(int argc, char* argv[])
{
    IAviReadFile* file=0;
    int64_t excess = 4000;
    WAVEFORMATEX wfmtx;
    int fd;
    if (argc > 1)
	filename = argv[1];

    char zz[100000];

    fd = open("/tmp/outoftest", O_WRONLY|O_CREAT|O_TRUNC, 00666);
    for (int i = 0; i < 1; i++)
    {
	file = CreateIAviReadFile(filename);
	if (!file)
            break;
	IAviReadStream* ars = file->GetStream(0, AviStream::Audio);
	if (ars && ars->StartStreaming() == 0)
	{
	    //ars->GetAudioFormatInfo(&wfmtx, 0);
	    int counter = 100;
	    while (counter-- > 0 && !ars->Eof())
	    {
		uint_t samples_read, bytes_read;
		ars->ReadFrames(zz, (excess > sizeof(zz)) ? excess : sizeof(zz),
				sizeof(zz), samples_read, bytes_read);
                if (fd >= 0)
		    write(fd, zz, bytes_read);
		//avm_usleep(000000);
		printf("counter %d  %d\n", counter, bytes_read);

	    }
	    ars->StopStreaming();
	}
	delete file;
	file = 0;
    }
    delete file;
    return 0;
}
