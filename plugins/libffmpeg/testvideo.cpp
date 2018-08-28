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
#include <avm_cpuinfo.h>
#include <utils.h>

#include <stdio.h>

static const char* filename =
// "/net/hdd2/movies/test/wm8_vandread.wmv";
// "/net/hdd2/movies/test/Lucky.asf";
// "/net/hdd2/movies/test3/imaadcmp.avi";
// "/net/hdd2/movies/test2/tm20.avi";
// "/net/hdd2/movies/test7/p_mjpg.avi";
// "/net/hdd2/movies/test7/pimj.avi";
// "/net/hdd2/movies/test/Tool_Sober.avi";
// "/net/hdd2/movies/test/Gone.avi";
    "/net/hdd2/movies/test2/avid.avi";
// "/net/hdd2/movies/test/fs306fs-20meg.avi";
static double tot_time = 0.0;

void testspeed(const char* fn)
{
    IAviReadFile* file = CreateIAviReadFile(fn);
    if (!file)
    {
	fprintf(stderr, "can't open file: %s\n", fn);
	return;
    }
    IAviReadStream* vrs = file->GetStream(0, AviStream::Video);
    if (!vrs)
    {
	fprintf(stderr, "no video stream in %s\n", fn);
        delete file;
	return;
    }
    if (vrs->StartStreaming() == 0)
    {
	//vrs->GetDecoder()->SetDestFmt(0, fccUYVY);
	//vrs->GetDecoder()->SetDestFmt(0, fccUYVY);
	//vrs->GetDecoder()->SetDestFmt(24);
	BitmapInfo(vrs->GetDecoder()->GetDestFmt()).Print();
	vrs->Seek(702);
	int counter = 50000;
	while (counter-- > 0 && !vrs->Eof())
	{
	    uint_t samples_read, bytes_read;
	    int64_t x = longcount();
	    vrs->ReadFrame(false);
	    tot_time += to_float(longcount(), x);
	    //printf("read %d  %d\n", counter, vrs->Eof());
	}
	vrs->StopStreaming();
    }
    printf("Total decompression time: %f\n", tot_time);
    delete file;
}

void testwrite(const char* fn)
{
    IAviReadFile* file = CreateIAviReadFile(fn);
    if (!file)
    {
	fprintf(stderr, "can't open file: %s\n", fn);
	return;
    }
    IAviReadStream* vrs = file->GetStream(0, AviStream::Video);
    if (!vrs)
    {
	fprintf(stderr, "no video stream in %s\n", fn);
        delete file;
	return;
    }

    //IAviReadStream*

    if (vrs->StartStreaming() == 0)
    {
	//vrs->GetDecoder()->SetDestFmt(0, fccUYVY);
	//vrs->GetDecoder()->SetDestFmt(0, fccUYVY);
	//vrs->GetDecoder()->SetDestFmt(24);
	BitmapInfo(vrs->GetDecoder()->GetDestFmt()).Print();
	vrs->Seek(702);
	int counter = 50000;
	while (counter-- > 0 && !vrs->Eof())
	{
	    uint_t samples_read, bytes_read;
	    int64_t x = longcount();
	    vrs->ReadFrame(false);
	    tot_time += to_float(longcount(), x);
	    //printf("read %d  %d\n", counter, vrs->Eof());
	}
	vrs->StopStreaming();
    }
    printf("Total decompression time: %f\n", tot_time);
    delete file;
}


extern "C" struct kprof_thread* kprof_init(void);
int main(int argc, char* argv[])
{
    //kprof_init();
    if (argc > 1)
        filename = argv[1];

    char zz[100000];
    for (int i = 0; i < 1; i++)
	testspeed(filename);

    return 0;
}
