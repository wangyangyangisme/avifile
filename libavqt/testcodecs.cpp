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
 *    mpatrol --show-all --leak-table testcodecs
 * with libefence:
 *    LD_PRELOAD=/usr/lib/libefence.so.0 testcodecs
 *
 */

#include <image.h>
#include <avm_except.h>
#include <avm_creators.h>
#include <avm_fourcc.h>
#include <infotypes.h>
#include <videoencoder.h>

#ifdef USE_MPATROL
#include <mpatrol.h>
#endif

#include <stdio.h>

#define __MODULE__ "testcl"

#include "codecdialog.h"

#include <qapplication.h>

int main(int argc, char** argv)
{

    BITMAPINFOHEADER bih;
    bih.biCompression = 0xffffffff;
    // just to fill video_codecs list
    avm::CreateDecoderVideo(bih, 0, 0);

    QApplication a( argc, argv );

    QavmCodecDialog* m = new QavmCodecDialog( 0, video_codecs, avm::CodecInfo::Both );

    return m->exec();
    //a.setMainWidget(m);
    return a.exec();


    for(int round = 0; round < 5; round++)
    {
        int i = 0;
	avm::VideoEncoderInfo _info;
	avm::vector<avm::CodecInfo>::iterator it;
	avm::vector<avm::CodecInfo> private_list;
	private_list.clear();
	fourcc_t fcc = 24;
	switch (round % 3)
	{
	case 1: fcc = fccYUY2; break;
	case 2: fcc = fccYV12; break;
	}
	avm::BitmapInfo bi(160, 120, fcc);
	printf("VideoCodes list size: %d\n",  video_codecs.size());
	for (it = video_codecs.begin(); it != video_codecs.end(); it++)
	{
	    if(!(it->direction & avm::CodecInfo::Encode))
		continue;
	    avm::IVideoEncoder* enc = avm::CreateEncoderVideo(it->fourcc, bi);
	    // checking if this code is the one we have asked for...
	    if (!enc)
		continue;
	    bool ok = (strcmp(it->GetName(), enc->GetCodecInfo().GetName()) == 0);
	    avm::FreeEncoderVideo(enc);
	    if (!ok)
		continue;
	    private_list.push_back(*it);
	    i++;
	}

	printf("round: %d  (fcc: %d)   found: %d\n", round, fcc, i);
    }
    return 0;
}
