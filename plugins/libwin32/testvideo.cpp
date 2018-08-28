/**
 *
 * Test sample for checking memory leaks in codec quering
 *
 * User should have mpatrol library installed
 * (In case it's not available - modify Makefile.am apropriately)
 * It's good idea to use single threaded operation mode
 * so modify Cache.cpp to no use thread for precaching #define NOTHREADS
 *
 * to use this program with mpatrol:
 *    mpatrol --show-all --leak-table testvideo
 * with libefence:
 *    LD_PRELOAD=/usr/lib/libefence.so.0 testvideo
 *
 */

#include <avifile.h>
#include <avm_except.h>
#include <avm_fourcc.h>
#include <utils.h>

#ifdef USE_MPATROL
#include <mpatrol.h>
#endif

#include <stdio.h>

const char* filename = "/net/hdd2/movies/test4/vp30_logo.avi";
//const char* filename = "/net/hdd2/movies/test-asf/wm8_vandread.wmv";
//const char* filename = "/net/hdd2/movies/test/Lucky.asf";
//const char* filename = "/net/hdd2/movies/test3/imaadcmp.avi";
//const char* filename = "/net/hdd2/movies/test2/tm20.avi";
//const char* filename = "/net/hdd2/movies/test7/p_mjpg.avi";
//const char* filename = "/net/hdd2/movies/test7/pimj.avi";
//const char* filename = "/net/hdd2/movies/test/Tool_Sober.avi";
//const char* filename = "/net/hdd2/movies/wm9/xaa";
//const char* filename = "/home/kabi/avi/wm9.asf";
//const char* filename = "/net/hdd2/movies/test/Gone.avi";

int main(int argc, char* argv[])
{
    IAviReadFile* file=0;

    if (argc > 1)
        filename = argv[1];

    char zz[100000];

    for (int i = 0; i < 1; i++)
    {
	file = CreateIAviReadFile(filename);
	if (!file)
            break;
	IAviReadStream* vrs = file->GetStream(0, AviStream::Video);
	if (vrs->StartStreaming() == 0)
	{
	    vrs->GetDecoder()->SetDestFmt(0, fccYV12);
	    //vrs->GetDecoder()->SetDestFmt(0, fccUYVY);
	    //vrs->GetDecoder()->SetDestFmt(24);
	    int counter = 3;//00;
	    while (counter-- > 0 && !vrs->Eof())
	    {
		uint_t samples_read, bytes_read;
		CImage* ci = vrs->GetFrame(true);
		if (ci)
		{
		    ci->Release();
		    printf("read %d\n", counter);
		}
	    }
	    vrs->StopStreaming();
	}
	delete file;
	file = 0;
    }
    delete file;
    return 0;
}
