#include <creators.h>
#include <except.h>
#include <stdio.h>

using namespace std;
using namespace Creators;
#define __MODULE__ "plustest"
int main()
{
    IVideoEncoder* enc=0;
    FILE* f1=0, *f2=0;
    char* compressed=0;
    try
    {
        f1=fopen("./uncompr.bmp", "rb");
	if(f1==0)
	    throw FATAL("This program expects to find 384x288x24 file ./uncompr.bmp");
	char h[14];	    
	fread(h, 14, 1, f1);
        //char bh[0x28];
	BITMAPINFOHEADER bh;
	BITMAPINFOHEADER obh;	
	fread(&bh, 0x28, 1, f1);
        char pic[384*288*3];
    	fread(pic, 384*288*3, 1, f1);
//	bh.biWidth=bh.biHeight=200;
	CImage im((BitmapInfo*)&bh, (unsigned char*)pic, false);
//        enc=CreateVideoEncoder(mmioFOURCC('d', 'v', 'x', '1'), bh);
//        enc=CreateVideoEncoder(mmioFOURCC('M', 'P', '0', '1'), bh);
	const CodecInfo* ci=CodecInfo::match(mmioFOURCC('D', 'I', 'V', '3'));
	if(ci)
	{
	    SetCodecAttr(*ci, "KeyFrames", 1);
	    SetCodecAttr(*ci, "BitRate", 1000);
	    SetCodecAttr(*ci, "Crispness", 0);
	}	    
	
        enc=CreateVideoEncoder(mmioFOURCC('D', 'I', 'V', '3'), bh);

	if(!enc)throw FATAL(GetError().c_str());
//	enc->SetKeyFrame(10);
//	enc->SetQuality(8000);
        int size=enc->GetOutputSize();
	if(size<=0)
	    throw FATAL("GetOutputSize() failed");
        else 
    	    printf("Driver reported %d as a maximum output size\n", size);
	compressed=new char[size];
        enc->Start();
	obh=enc->QueryOutputFormat();
	int keyf;
        for(int i=0; i<30; i++)
	{
    	    if(enc->EncodeFrame(&im, compressed, &keyf, &size)!=0)
	    {
		printf("EncodeFrame() failed\n");
            }
	    else printf("Frame %d encoded as %d ( %s ) into %d bytes\n", i, keyf, keyf&16?"Key frame":"Delta frame", size);
        }
	enc->Stop();
        f2=fopen("./compr.qw", "wb");
	fwrite(&obh, sizeof(BITMAPINFOHEADER), 1, f2);
	fwrite(compressed, size, 1, f2);
	fclose(f1);
	fclose(f2);f2=0;
	f1=fopen("./decompr.bmp", "wb");
	fwrite(h, 14, 1, f1); 
	fwrite(&bh, sizeof(BITMAPINFOHEADER), 1, f1);
	IVideoDecoder* dc=CreateVideoDecoder(obh);
	if(!dc)throw FATAL(GetError().c_str());
	dc->SetDirection(true);
	dc->Start();
//	dc->SetDestFmt(0, fccYUY2);
//	dc->SetDestFmt(16);
	dc->DecodeFrame(compressed, size, keyf);
	CImage* img=dc->GetFrame();
	fwrite(img->Data(), img->Bytes(), 1, f1);
	img->Release();
	dc->Stop();
	FreeVideoDecoder(dc);
    }
    catch(FatalError& error)
    {
	error.PrintAll();
    }	
    if(f1)fclose(f1);
    if(f2)fclose(f2);
    FreeVideoEncoder(enc);
    if(compressed)delete compressed;
    return 0;
}
