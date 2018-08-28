/**************************************************************************

	Example of IAviWriteFile & IAviVideoWriteStream interface usage.
	
**************************************************************************/


#include "avifile.h"
#include "aviplay.h"
#include "avm_cpuinfo.h"
#include "avm_except.h"
#include "avm_fourcc.h"
#include "videoencoder.h"
#include "utils.h"
#include "version.h"


#include <iostream>
#define __MODULE__ "avitest"

#include <cstdio>
#include <cstring>

using namespace std;

int main(int argc, char** argv)
{
//    int fccHandler=mmioFOURCC('m','j','p','g');
    int fccHandler = fccdiv1;
//    int fccHandler=mmioFOURCC('A', 'P', '4', '1');
//    int fccHandler=fccDIV3;
//    IVideoEncoder::SetExtendedAttr(fccHandler, "QuickCompress", 0);
    avm::IWriteFile* file = 0;
    FILE* f1 = 0;
    if(GetAvifileVersion()!=AVIFILE_VERSION)
    {
	cout<<"This binary was compiled for Avifile ver. "<<AVIFILE_VERSION<<", but the library is ver. "<<GetAvifileVersion()<<". Aborting."<<endl;
	return 0;
    }	
    if(argc!=2)
    {
	printf("Usage: %s <output file>\n", argv[0]);
	return 0;
    }	
    try
    {
	file=avm::CreateWriteFile(argv[1]);
//	file=CreateSegmentedFile(argv[1], 400000);

	f1=fopen("./uncompr.bmp", "rb");
	if(f1==0)throw FATAL("Can't open file ./uncompr.bmp");
        fseek(f1, 14, SEEK_SET);//BITMAPFILEHEADER
	BITMAPINFOHEADER bh;
	fread(&bh, 0x28, 1, f1);
//	bh.biWidth=0x80;
//	bh.biHeight=0x80;
//	bh.biBitCount=32;
//        VideoEncoder::SetExtendedAttr(fccDIV3, "BitRate", 4500);//before we initialize decoder
//        IVideoEncoder::SetExtendedAttr(fccIV50, "QuickCompress", 1);
	
	avm::IVideoWriteStream* stream=file->AddVideoStream(fccHandler, &bh, 100000);
        unsigned char pic[384*288*3];
    	fread(pic, 384*288*3, 1, f1);
	avm::CImage im((avm::BitmapInfo*)&bh, pic, false);

//	stream->SetHeader(40000);//mks/frame
	stream->Start();
	long long t1=longcount();
        for(int i=0; i<144; i++)
	{
	    stream->AddFrame(&im);
    	    memset(pic+384*i*3, 0, 384*3);
	    memset(pic+384*(287-i)*3, 0, 384*3);
	    if(i%10==0)
		printf("%d frames written\n", i);
	}
	long long t2=longcount();
	cerr<<to_float(t2, t1) * 1000.0 <<" ms per frame"<<endl;
        stream->Stop();
	delete file;//here all headers are actually written
        fclose(f1);
	return 0;
    }
    catch(FatalError& error)
    {
	error.Print();
	if(file)delete file;
	if(f1)fclose(f1);
    }
    catch(...)
    {
	cout<<"ERROR: Caught unknown exception!"<<endl;
    }		
    return 0;
}    
