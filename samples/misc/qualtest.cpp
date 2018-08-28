/**
 * Quick & dirty way to compare quality of different video encoders.
 * Example:
 *  qualtest -f your-movie-file.avi -t 20. -div3 600
 * recompresses one second of movie, starting from 20 second position,
 * into divx low-motion with bitrate 600 kbps, dumps two screenshots:
 * 'reference.bmp' for uncompressed pix and 'div3_600.bmp' for result of
 * recompression, and tells you about actual achieved bitrate.
 * Supported formats: DivX low-motion, OpenDivX & Sparky.
 */

#include <default.h>
#include <avifile.h>
#include <videoencoder.h>
#include <creators.h>

#include <unistd.h>
#include <iostream>
#include <cmath>
#include <cstdio>


using namespace std;
#define fccDIVX mmioFOURCC('d', 'i', 'v', 'x')
const int FRAMES=25;
struct Result
{
    int quality;
    int bitrate;    
};
inline short clamp(short t)
{
    return((t<0)?0:((t>255)?255:t));
}

ostream& operator<<(ostream& o, Result r)
{
    return o<<" error level "<<double(r.quality)/(FRAMES*384*288)<<", bitrate "<<8*r.bitrate*(25./FRAMES)<<" bits/s"<<endl;
}
int diff(CImage* im, CImage* im2)
{
    int s=0;
    for(int i=0; i<im->Bytes(); i++)
	s+=abs((int)im->Data()[i]-(int)im2->Data()[i]);
    return s;
}
CImage* filter(CImage* image)
{
    static int call=0;
    call++;
    int addition=5*sin(call/10.);
    CImage* result=new CImage(image);
    for(int i=1; i<image->Width()-1; i++)
	for(int j=1; j<image->Height()-1; j++)
	{
	    col* pc1=(col*)(image->Data()+i*3+j*3*image->Width()-3);
	    col* pc=(col*)(image->Data()+i*3+j*3*image->Width());
	    col* pc2=(col*)(image->Data()+i*3+j*3*image->Width()+3);
	    col* dst=(col*)(result->Data()+i*3+j*3*image->Width());
	    dst->r=clamp(((short)pc1->r+3*(short)pc->r+(short)pc2->r)*0.27+addition);
	    dst->g=clamp(((short)pc1->g+3*(short)pc->g+(short)pc2->g)*0.27+addition);
	    dst->b=clamp(((short)pc1->b+3*(short)pc->b+(short)pc2->b)*0.27+addition);
	}    
    return result;
}

int get_im_color(int i, int j)
{
    return ((i/2)*123+(j/2)*459)%32;// + 3*(i%2) + 3*(j%2);
}
//yuy2
/*
int diff(CImage* im1, CImage* im2, int x1, int y1, int x2, int y2)
{
    int sum=0;
    unsigned char* ptr1=im1->Data()+2*(x1+y1*im1->Width());
    unsigned char* ptr2=im2->Data()+2*(x2+y2*im2->Width());
    for(int i=0; i<8; i++)
    {
	for(int j=0; j<8; j++)
	    sum+=abs((short)ptr1[2*j]-(short)ptr2[2*j]);
	ptr1+=2*im1->Width();
	ptr2+=2*im2->Width();
    }
    return sum;
}
*/
void InitVectorImage(CImage* vec_im)
{
    memset(vec_im->Data(), 128, vec_im->Bytes());
    int i,j;
    for(i=0; i<vec_im->Width()/8; i++)
	for(j=0; j<vec_im->Height()/8; j++)
	{
	    int im_color=get_im_color(i,j);
	    for(int k=0; k<8; k++)
		for(int l=0; l<8; l++)
		vec_im->Data()[2*((i*8+l)+(j*8+k)*vec_im->Width())]
		    =im_color+128+32*cos(k*3.14159/8.)*(i%2)
		    +32*cos(l*3.14159/8.)*(j%2);
	}
}
 
void match(CImage* orig, CImage* tmp_im2, int i, int j, int& x_opt, int& y_opt)
{
    x_opt=y_opt=0;
    int bestval=4096*256;
    const int width=tmp_im2->Width();
    for(int x=-8; x<=8; x++)
	for(int y=-8; y<=8; y++)
	{
	    int sum=0;
	    unsigned char* ptr1=orig->Data()+2*((i*8+x)+(j*8+y)*width);
	    unsigned char* ptr2=tmp_im2->Data()+2*(i*8+j*8*width);
	    for(int k=0; k<8; k++)
	    {
		for(int l=0; l<8; l++)
		{
		    short dif=(short)ptr1[2*l]-(short)ptr2[2*l];
		    if(dif<0)
			sum-=dif;
		    else
			sum+=dif;
		}
		if(sum>bestval)
		    goto next;
		ptr1+=2*width;
		ptr2+=2*width;
	    }
	    if(sum<bestval)
	    {
		bestval=sum;
		x_opt=x;
		y_opt=y;
	    }
	    if(sum==bestval && (abs(x)+abs(y)<abs(x_opt)+abs(y_opt)))
	    {
		x_opt=x;
		y_opt=y;
	    }
	next:
	    ;
	}    
}

void dump_vectors(char* buf, int size, char* gray_buffer, int gray_size, char* vec_buffer, int vec_size, IVideoDecoder* dc)
{
//    CImage* tmp_im=new CImage(unc, &bi);
    dc->Start();
    dc->DecodeFrame(vec_buffer, vec_size, 16);
    CImage* im=dc->GetFrame();
    BitmapInfo bi(im->Width(), im->Height(), fccYUY2);
    CImage* orig=new CImage(im, &bi);
    orig->Dump("orig.bmp");
    im->Release();
    dc->DecodeFrame(buf, size, 0);
    im=dc->GetFrame();
    im->Dump("vec_diff.bmp");
    CImage* tmp_im2=new CImage(im, &bi);
    im->Release();
    dc->Stop();

    dc->Start();
    dc->DecodeFrame(gray_buffer, gray_size, 16);
    dc->DecodeFrame(buf, size, 0);
    im=dc->GetFrame();
    CImage* tmp_im3=new CImage(im, &bi);
    im->Release();
    dc->Stop();
    
    int sum=0;
    yuv y(col(128,128,128));
    int i,j;
    for(i=0; i<tmp_im2->Bytes(); i++)
	tmp_im2->Data()[i]+=((i%2)?y.Cr:y.Y)-(short)tmp_im3->Data()[i];
    tmp_im2->Dump("vec_diff2.bmp");
//    CImage* orig=new CImage(tmp_im2);
//    InitVectorImage(orig);
    bi.SetSpace(fccYV12);
    CImage* vectors=new CImage(&bi);
    memset(vectors->Data(), 128, vectors->Width()*vectors->Height());    
    for(i=16; i<tmp_im2->Width()-16; i+=8)
	for(j=16; j<tmp_im2->Height()-16; j+=8)
	{
	    int x_opt, y_opt;
	    match(orig, tmp_im2, i/8, j/8, x_opt, y_opt);
	    for(int k=0; k<4; k++)
	    {
		memset(vectors->Data()+vectors->Width()*vectors->Height()
		    +i/2+(j/2+k)*vectors->Width()/2,
		    128+x_opt*4, 4);
		memset(vectors->Data()+5*vectors->Width()*vectors->Height()/4
		    +i/2+(j/2+k)*vectors->Width()/2,
		    128+y_opt*4, 4);
	    }
	}
    
    orig->Release();
    vectors->Dump("vectors-pred.bmp");
    vectors->Release();
    tmp_im2->Release();
    dc->Stop();
}

void dump_gray(CImage* unc, char* buf, int size, char* gray_buffer, int gray_size, IVideoDecoder* dc)
{
    BitmapInfo bi(unc->Width(), unc->Height(), fccYUY2);
    CImage* tmp_im=new CImage(unc, &bi);
    dc->Start();
    dc->DecodeFrame(gray_buffer, gray_size, 16);
    dc->DecodeFrame(buf, size, 0);
    CImage* im=dc->GetFrame();
    im->Dump("diff.bmp");
    CImage* tmp_im2=new CImage(im, &bi);
    int sum=0;
    yuv y(col(128,128,128));
    for(int i=0; i<tmp_im->Bytes(); i++)
    {
	tmp_im->Data()[i]+=0x80-(short)tmp_im2->Data()[i];
	if(i%2)
	    sum+=abs(y.Cr-(short)tmp_im2->Data()[i]);
	else
	    sum+=abs(y.Y-(short)tmp_im2->Data()[i]);
    }
    printf("/%d ", sum);
    tmp_im->Dump("compens-pred.bmp");
    tmp_im2->Release();
    tmp_im->Release();	
    im->Release();
    dc->Stop();
}

IAviReadStream* stream;
double startpos=297.;
bool exact=true;
Result measure(int compressor, const char* filename)
{
//    stream->SeekTimeToKeyframe(599.0);
//    stream->SeekTimeToKeyframe(55.0);
    stream->SeekTimeToKeyFrame(startpos);
    stream->SetDirection(true);
    stream->StartStreaming();
//    stream->GetDecoder()->SetDestFmt(0, fccYUY2);
    if(exact)
	while(stream->GetTime()<startpos)
	    stream->ReadFrame();

    IRtConfig* rtc=dynamic_cast<IRtConfig*>(stream->GetDecoder());
    if(rtc)rtc->SetValue("Quality", 4);
    
    stream->ReadFrame();
    CImage* im=stream->GetFrame();
//    CImage* filtered=filter(im);
//    im->Release();
//    im=filtered;
    char gray_buffer[100000];
    int gray_size;
    char vec_buffer[100000];
    int vec_size;
    IVideoEncoder* enc=Creators::CreateVideoEncoder(compressor, *(im->GetFmt()));
    if(!enc){im->Release();return Result();}
    IVideoDecoder* dc=Creators::CreateVideoDecoder(enc->GetOutputFormat());
    if(!dc){im->Release();Creators::FreeVideoEncoder(enc);return Result();}
//    IVideoDecoder* dc2=Creators::CreateVideoDecoder(enc->GetOutputFormat());

    char* buf=new char[enc->GetOutputSize()];
    dc->SetDirection(true);
//    dc2->SetDirection(true);
    dc->Start();
//    enc->Start();
    
    int size=0, read_size=0, is_keyframe=0;
    int qual=0;
//    CImage* gray=new CImage(im);
//    memset(gray->Data(), 128, gray->Bytes());
//    enc->EncodeFrame(gray, gray_buffer, &is_keyframe, &gray_size);
//    enc->Stop();
    BitmapInfo bi(im->Width(), im->Height(), fccYUY2);
//    CImage* vec_im=new CImage(&bi);
//    InitVectorImage(vec_im);
//    enc->Start();
//    enc->EncodeFrame(vec_im, vec_buffer, &is_keyframe, &vec_size);
//    vec_im->Release();
//    enc->Stop();
    enc->Start();
    enc->EncodeFrame(im, buf, &is_keyframe, &size);
    read_size+=size;
    dc->DecodeFrame(buf, size, is_keyframe);
    CImage* im2=dc->GetFrame();
    qual+=diff(im, im2);
    im->Release();
    im2->Release();
    for(int i=0; i<FRAMES; i++)
    {
    stream->ReadFrame();
    im=stream->GetFrame();
//    filtered=filter(im);
//    im->Release();
//    im=filtered;
    im->Dump("reference.bmp");
    enc->EncodeFrame(im, buf, &is_keyframe, &size);
    if(compressor!=fccDIV3)
	cout<<size<<" "<<flush;
    else
	cout<<size<<"("<<((buf[0]>>1)&31)<<")"<<flush;
    read_size+=size;
    dc->DecodeFrame(buf, size, is_keyframe);
    im2=dc->GetFrame();
//    dump_gray(im2, buf, size, gray_buffer, gray_size, dc2);
//    dump_vectors(buf, size, gray_buffer, gray_size, vec_buffer, vec_size,  dc2);
    cout<<" "<<flush;
    qual+=diff(im, im2);
    im2->Dump(filename);
    im->Release();
    im2->Release();
    }
    
    Result res;
    res.bitrate=read_size;
    res.quality=qual;    
    cout<<endl;
    cout<<filename<<": "<<res<<endl;
    stream->StopStreaming();
//    Creators::FreeVideoDecoder(dc2);
    Creators::FreeVideoDecoder(dc);
    Creators::FreeVideoEncoder(enc);
    delete[] buf;
    return res;
}
void measure_sparky(int intraq, int interq)
{	
    char outfn[256];
    if(interq==intraq)
	sprintf(outfn, "spark_%d.bmp", intraq);
    else
	sprintf(outfn, "spark_%d_%d.bmp", intraq, interq);
	
    int fcc=mmioFOURCC('S', 'P', '0', '1');
    const CodecInfo* info=CodecInfo::match(fcc);

    Creators::SetCodecAttr(*info, "IntraQ", intraq);
    Creators::SetCodecAttr(*info, "InterQ", interq);
    measure(fcc, outfn);
}
void measure_div3(int bitrate)
{
    char outfn[256];
    sprintf(outfn, "div3_%d.bmp", bitrate);
    const CodecInfo* info=CodecInfo::match(fccDIV3);

    Creators::SetCodecAttr(*info, "BitRate", bitrate);
    Creators::SetCodecAttr(*info, "Crispness", 0);
    measure(fccDIV3, outfn);
}

void measure_divx(int quant)
{
    char outfn[256];
    sprintf(outfn, "divx_%d.bmp", quant);
    const CodecInfo* info=CodecInfo::match(fccDIVX);

    Creators::SetCodecAttr(*info, "min_quantizer", quant);
    Creators::SetCodecAttr(*info, "max_quantizer", quant);
    measure(fccDIVX, outfn);
}

int main(int argc, char** argv)
{
    startpos=297.;
    exact=true;
    char* fn="/d/drafts/movie.avi";
    int i;
    for(i=1; i<argc; i++)
    {
	if(!strcmp(argv[i], "-t"))
	{
	    i++;
	    sscanf(argv[i], "%lf", &startpos);
	}
	if(!strcmp(argv[i], "-e"))
	    exact=false;
	if(!strcmp(argv[i], "-f"))
	{
	    i++;
	    fn=argv[i];
	}
    }
    
    IAviReadFile* file=CreateIAviReadFile(fn);
//    IAviReadFile* file=CreateIAviReadFile("/d/movies/mission2mars.avi");
    stream=file->GetStream(0, AviStream::Video);
    const CodecInfo* info=CodecInfo::match(fccIV50);
    Creators::SetCodecAttr(*info, "QuickCompress", 0);
    info=CodecInfo::match(fccDIV3);
    Creators::SetCodecAttr(*info, "Hue", 50);
    Creators::SetCodecAttr(*info, "Saturation", 50);
    
    for(i=1; i<argc; i++)
    {
	if(!strcmp(argv[i], "-sparky"))
	{
	    i++;
	    int arg1=atoi(argv[i]);
	    if(i+1<argc)
	    {
		int arg2=atoi(argv[i+1]);
		measure_sparky(arg1, arg2);
		i++;
	    }
	    else
		measure_sparky(arg1, arg1);
	}
	if(!strcmp(argv[i], "-div3"))
	{
	    i++;
	    measure_div3(atoi(argv[i]));
	}
	if(!strcmp(argv[i], "-divx"))
	{
	    i++;
	    measure_divx(atoi(argv[i]));
	}
    }
    
//    Result res;
//    FILE* stat=fopen("./stat1", "w");
//    int fcc=mmioFOURCC('S', 'P', '0', '1');
//    info=CodecInfo::match(fcc);

//    Creators::SetCodecAttr(*info, "IntraQ", 11);
//    Creators::SetCodecAttr(*info, "InterQ", 11);
//    res=measure(fcc, 10000, 10, "spark_11.bmp");

//    Creators::SetCodecAttr(*info, "IntraQ", 15);
//    Creators::SetCodecAttr(*info, "InterQ", 15);
//    res=measure(fcc, 10000, 10, "spark_15.bmp");

//    Creators::SetCodecAttr(*info, "IntraQ", 16);
//    Creators::SetCodecAttr(*info, "InterQ", 16);
//    res=measure(fcc, 10000, 10, "spark_16.bmp");

//    Creators::SetCodecAttr(*info, "IntraQ", 22);
//    Creators::SetCodecAttr(*info, "InterQ", 22);
//    res=measure(fcc, 10000, 10, "spark_22.bmp");


/*
    Creators::SetCodecAttr(*info, "IntraQ", 8);
    Creators::SetCodecAttr(*info, "InterQ", 8);
    res=measure(fcc, 10000, 10, "spark_8.bmp");

    Creators::SetCodecAttr(*info, "IntraQ", 9);
    Creators::SetCodecAttr(*info, "InterQ", 9);
    res=measure(fcc, 10000, 10, "spark_9.bmp");
*/

//    Creators::SetCodecAttr(*info, "IntraQ", 10);
//    Creators::SetCodecAttr(*info, "InterQ", 10);
//    res=measure(fcc, 10000, 10, "spark_10.bmp");

//    Creators::SetCodecAttr(*info, "IntraQ", 15);
//    Creators::SetCodecAttr(*info, "InterQ", 15);
//    res=measure(fcc, 10000, 10, "spark_15.bmp");
    
//    Creators::SetCodecAttr(*info, "IntraQ", 19);
//    Creators::SetCodecAttr(*info, "InterQ", 19);
//    res=measure(fcc, 10000, 10, "spark_19.bmp");
    
//    Creators::SetCodecAttr(*info, "IntraQ", 25);
//    Creators::SetCodecAttr(*info, "InterQ", 25);
//    res=measure(fcc, 10000, 10, "spark_25.bmp");
    
//    Creators::SetCodecAttr(*info, "IntraQ", 30);
//    Creators::SetCodecAttr(*info, "InterQ", 30);
//    res=measure(fcc, 10000, 10, "spark_30a.bmp");
    
//    Creators::SetCodecAttr(*info, "IntraQ", 35);
//    Creators::SetCodecAttr(*info, "InterQ", 35);
//    res=measure(fcc, 10000, 10, "spark_35a.bmp");

//    info=CodecInfo::match(fccDIV3);

//    Creators::SetCodecAttr(*info, "BitRate", 850);
//    res=measure(fccDIV3, 10000, 10, "div3_850.bmp");

//    Creators::SetCodecAttr(*info, "BitRate", 2000);
//    res=measure(fccDIV3, 10000, 10, "div3_2000.bmp");

//    Creators::SetCodecAttr(*info, "BitRate", 1500);
//    res=measure(fccDIV3, 10000, 10, "div3_1500.bmp");

//    info=CodecInfo::match(fccDIVX);

//    Creators::SetCodecAttr(*info, "BitRate", 500000);
//    res=measure(fccDIVX, 10000, 10, "divx_500.bmp");

//    Creators::SetCodecAttr(*info, "BitRate", 720000);
//    res=measure(fccDIVX, 10000, 10, "divx_720.bmp");

//    info=CodecInfo::match(fccDIVX);
//    Creators::SetCodecAttr(*info, "BitRate", 20000);
//    res=measure(fccDIVX, 10000, 10, "divx_20.bmp");

//    Creators::SetCodecAttr(*info, "BitRate", 400000);
//    res=measure(fccDIVX, 10000, 10, "divx_400.bmp");

//    Creators::SetCodecAttr(*info, "BitRate", 580000);
//    res=measure(fccDIVX, 10000, 10, "divx_580.bmp");
    
/*
    info=CodecInfo::match(fccDIVX);
    Creators::SetCodecAttr(*info, "BitRate", 1000000);
    res=measure(fccDIVX, 10000, 10, "divx_1000.bmp");
//    Creators::SetCodecAttr(*info, "BitRate", 1200000);
//    res=measure(fccDIVX, 10000, 10, "divx_1200.bmp");
    Creators::SetCodecAttr(*info, "BitRate", 1500000);
    res=measure(fccDIVX, 10000, 10, "divx_1500.bmp");
    Creators::SetCodecAttr(*info, "BitRate", 2000000);
    res=measure(fccDIVX, 10000, 10, "divx_2000.bmp");
*/  

    return 0;
}
