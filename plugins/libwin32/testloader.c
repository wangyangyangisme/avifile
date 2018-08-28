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

#ifdef USE_MPATROL
#include <mpatrol.h>
#endif

#include <stdlib.h>
#include <stdio.h>

static const char* outf = "/tmp/x";

int main(int argc, char* argv[])
{
    FILE* f = fopen(outf, "rb");

    if (!f)
	return -1;

    //driver

    while (!feof(f))
    {
	unsigned sz;
        char* tbuf;

	fread(&sz, sizeof(sz), 1, f);
	tbuf = (char*)malloc(sz);
	fread(tbuf, sz, 1, f);


	free(tbuf);
    }
    fclose(f);
    return 0;
}
