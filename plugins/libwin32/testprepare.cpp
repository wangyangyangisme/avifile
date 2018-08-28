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

//const char* filename = "/net/hdd2/movies/test/wm8_vandread.wmv";
//const char* filename = "/net/hdd2/movies/test/Lucky.asf";
//const char* filename = "/net/hdd2/movies/test3/imaadcmp.avi";
//const char* filename = "/net/hdd2/movies/test2/tm20.avi";
const char* filename = "/net/hdd2/movies/test7/p_mjpg.avi";
//const char* filename = "/net/hdd2/movies/test7/pimj.avi";
//const char* filename = "/net/hdd2/movies/test/Tool_Sober.avi";
//const char* filename = "/net/hdd2/movies/test2/avid.avi";

static const char* outf = "/tmp/x";

int main(int argc, char* argv[])
{
    IAviReadFile* file=0;
    FILE* f = fopen(outf, "wb");

    if (!f)
	return -1;

    if (argc > 1)
        filename = argv[1];

    file = CreateIAviReadFile(filename);
    if (!file)
	return 1;

    IAviReadStream* vrs = file->GetStream(0, AviStream::Video);

    int counter = 20;
    while (counter-- > 0 && !vrs->Eof())
    {
	uint_t lSize, lSamples, lBytes;
	int hr = vrs->ReadDirect(0, 0, 0, lSamples, lSize);
	printf("write %d\n", lSize); fflush(stdout);
	char* tbuf = new char[lSize * 2];
	vrs->ReadDirect(tbuf, lSize, 1, lSamples, lBytes);

	fwrite(&lSize, sizeof(lSize), 1, f);
	fwrite(tbuf, lBytes, 1, f);
	delete[] tbuf;
    }
    delete file;
    fclose(f);
    return 0;
}
