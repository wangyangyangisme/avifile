#include <avifile.h>

#include <encore2.h>
#include <decore.h>

#include <avm_fourcc.h>
#include <avm_except.h>
#include <avm_cpuinfo.h>
#include <avm_creators.h>
#include <utils.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // atoi
#include <string.h> // memset
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>

#include <iostream>

extern "C" 
{
    extern int quiet_encore;
}

using namespace std;


extern "C" const avm::vector<CodecInfo>& RegisterPlugin();
extern "C" const double frequency=700000.;

#define __MODULE__ "test codec"

bool no_encode=false;
bool no_decode=false;
bool quiet;

int intra_decisions;
int inter_decisions;
double avgsnr=0;
int BLOCK_DEBUG;
int MOTION_DEBUG;
int num_frames=5;
double ac_effect;
float start_time=599.;
bool use_deblock_filter;
int averager;
int quant=10;
int quant2=10;
bool no_16;
bool no_8;
bool draw=false;
bool use_stretcher;
extern "C" int neigh_tend_16x16;
extern "C" int neigh_tend_8x8;
extern "C" int32_t iMv16x16;
extern "C" int32_t mv_size_parameter;
static bool exact=false;
bool verify=true;    
int quality=9;
int yoff=0;
int intra_freq=300;
const char* filename="/c/trailers/matrix-div4-500.avi";
template <class T> inline T max(const T x, const T y) { return (x>y)?x:y; }

inline int get_dc_scaler(int qp, int i)
{
    if(qp<=4)return 8;
    if(i<4)
    {
	if(qp<=8)return 2*qp;
	if(qp<=25)return qp+8;
	return 2*qp-16;
    }
    if(qp<=25)return (qp+13)/2;
    return qp-6;
}

double diff(CImage* im1, CImage* im2)
{
    int sum=0;
    for(unsigned i=0; i<im1->Bytes(); i++)
	sum+=abs((int)im1->Data()[i]-(int)im2->Data()[i]);
    return (double)sum/im1->Bytes();
}

int main(int argc, char** argv)
{
    quiet=true;
    quiet_encore=0;
    for(int i=1; i<argc; i++)
    {
	if(!strcmp(argv[i], "-f"))
	{
	    i++;
	    filename=argv[i];	    
	    continue;
	}
	if(!strcmp(argv[i], "-t"))
	{
	    i++;
	    sscanf(argv[i], "%f", &start_time);
	}
	if(!strcmp(argv[i], "-quiet"))
	    quiet=true;
	if(!strcmp(argv[i], "-no-encode"))
	    no_encode=true;
	if(!strcmp(argv[i], "-no-decode"))
	    no_decode=true;
	if(!strcmp(argv[i], "-draw"))
	    draw=true;
	if(!strcmp(argv[i], "-no-16"))
	    no_16=true;
	if(!strcmp(argv[i], "-no-8"))
	    no_8=true;
	if(!strcmp(argv[i], "-no-stretcher"))
	    use_stretcher=false;
	if(!strcmp(argv[i], "-debug"))
	    BLOCK_DEBUG=MOTION_DEBUG=1;
	if(!strcmp(argv[i], "-no-verify"))
	    verify=false;
	if(!strcmp(argv[i], "-exact"))
	    exact=true;
	if(!strcmp(argv[i], "-qual"))
	{
	    i++;
	    quality=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-quant"))
	{
	    i++;
	    quant=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-quant2"))
	{
	    i++;
	    quant2=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-num-frames"))
	{
	    i++;
	    num_frames=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-averager"))
	{
	    i++;
	    averager=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-16"))
	{
	    i++;
	    iMv16x16=atoi(argv[i]);
	//    neigh_tend_16x16=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-neigh_16"))
	{
	    i++;
	    neigh_tend_16x16=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-neigh_8"))
	{
	    i++;
	    neigh_tend_8x8=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-mvsp"))
	{
	    i++;
	    mv_size_parameter=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-yoff"))
	{
	    i++;
	    yoff=atoi(argv[i]);
	}
	if(!strcmp(argv[i], "-ac-effect"))
	{
	    i++;
	    sscanf(argv[i], "%lf", &ac_effect);
	}
	if(!strcmp(argv[i], "-intra"))
	{
	    i++;
	    intra_freq=atoi(argv[i]);
	}
	
	if(!strcmp(argv[i], "-deblock"))
	    use_deblock_filter=true;
    }
    IAviReadFile* file=0;

    {
        int i;
	unsigned short w;
	unsigned short h;
	int fd;
        int progress=0;
	CImage* im;
	long long ttt=0;
	long long ttt2=0;
	long long ttt3=0;
	long long t1, t2;	    
	char* dest=new char[3000000];	
	intra_decisions=inter_decisions=0;

	/** Start reading uncompressed frames from file **/
	/** By default they are in BGR24 mode **/
	file=CreateIAviReadFile(filename);
	if (!file)
            return 1;
	IAviReadStream* stream=file->GetStream(0, AviStream::Video);
	if (!stream)
	{
	    printf("No video stream!\n");
            return 1;
	}
	stream->SetDirection(true);
	if(stream->StartStreaming())
	{
	    printf("Can't start streaming!\n");
            return 1;
	}
	stream->SeekTimeToKeyFrame((double)start_time);
//        stream->GetDecoder()->SetDestFmt(0, fccYV12);
	if(exact)
	    while(stream->GetTime()<start_time)
		stream->ReadFrame(false);
//	    IRtConfig* rtc=dynamic_cast<IRtConfig*>(stream->GetDecoder());
//	    if(rtc)rtc->SetValue("Quality", 4);

    	BitmapInfo bi=stream->GetDecoder()->GetDestFmt();
	w = bi.biWidth;
	h = labs(bi.biHeight);

	/** Create encoder **/
	ENC_PARAM param;
	memset(&param, 0, sizeof(param));
	param.x_dim=w;
	param.y_dim=h-yoff;
	param.framerate=25.;
	param.min_quantizer=quant;
	param.max_quantizer=quant;
	param.bitrate=600000;
	param.quality=quality;
	param.max_key_interval=intra_freq;
#ifndef ENCORE_MAJOR_VERSION	
	param.use_bidirect=1;
#else
	param.extensions.use_bidirect=1;
#endif
	encore(0, ENC_OPT_INIT, &param, 0);
	void* handle=param.handle;

	/** Create decoder **/
#if DECORE_VERSION >= 20021112
	DEC_INIT dp;
	decore(&handle, DEC_OPT_INIT, &dp, 0);
#else
        DEC_PARAM dp;
        dp.x_dim=w;
        dp.y_dim=h;
        dp.output_format=DEC_YUY2;
	memset(&dp.buffers, 0, sizeof(dp.buffers));
	decore(1, DEC_OPT_INIT, &dp, 0);
#endif
	    
        BitmapInfo bi2(w, h, fccYUY2);
        CImage* dec_out=new CImage(&bi2);
	for(i=0; i<num_frames; i++)
	{
	    t1=longcount();
	    stream->ReadFrame();
	    im=stream->GetFrame();
	    t2=longcount();
	    ttt3+=t2-t1;
	    char s[20];
	    if(draw)
	    {
	        sprintf(s, "unc-%03d.bmp", i);
	        im->Dump(s);
	    }
	    ENC_FRAME fr;
//#define DIRECT_ENCODING
//#define TEST_YUY2
#ifdef DIRECT_ENCODING
	    fr.image=im->Data();	    
	    fr.colorspace=ENC_CSP_YV12;
#else
#ifdef TEST_YUY2
	    BitmapInfo bi(im->Width(), im->Height(), fccYUY2);
	    fr.colorspace=ENC_CSP_YUY2;
#else
	    BitmapInfo bi(im->Width(), im->Height(), 24);
	    fr.colorspace=ENC_CSP_RGB24;
#endif
	    CImage* im2=new CImage(im, &bi);
	    fr.image=im2->Data();
#endif	    
	    fr.bitstream=dest;
	    fr.length=3000000;
	    fr.mvs=0;
	    if(i==0)
		fr.quant=quant;
	    else
		fr.quant=quant2;
	    fr.intra=-1;

	    ENC_RESULT res;
	    t1=longcount();
	    encore(handle, ENC_OPT_ENCODE_VBR, &fr, &res);
	    t2=longcount();
	    ttt+=t2-t1;

//	    im->Release();
#ifndef DIRECT_ENCODING
	    im2->Release();
#endif
	    printf("Frame %d encoded as %s into %d bytes\n",
	        i, res.is_key_frame?"INTRA":"INTER", fr.length);
	    progress+=fr.length;
	    if(i==0)
		printf("ATTN: quant %d->%d, text %d, non-text %d\n", 
		    quant, quant2, res.texture_bits, res.total_bits-res.texture_bits);

	    DEC_FRAME df;
	    df.bmp=dec_out->Data();
	    df.bitstream=dest;
	    df.length=fr.length;
	    df.render_flag=1;
	    df.stride=w;
	    t1=longcount();
#if DECORE_VERSION >= 20021112
	    decore(handle, DEC_OPT_INIT, &dp, 0);
#else
	    decore(1, DEC_OPT_FRAME, &df, 0);
#endif
	    t2=longcount();
	    ttt2+=t2-t1;
	    BitmapInfo bb(w, h, 24);
	    CImage* im3=new CImage(dec_out, &bb);
	    double snr=diff(im, im3);
	    im3->Release();
	    printf("SNR: %f ( %f db )\n", snr, 10*log(snr/256.)/log(10.));
	    avgsnr+=snr;
	    if(draw)
	    {
    	        sprintf(s, "dec-%03d.bmp", i);
	        dec_out->Dump(s);
	    }

	}	
	float qual_mul = 1;
	switch(quality)
	{
	case 5:
	case 4:
        	qual_mul=5;
		break;
	case 3:
	case 2:
		qual_mul=3.5;
		break;
	case 1:
		qual_mul=2;
		break;
	}
        cout<<"Encoded "<<i<<" frames ( "<<i*5*w*h/4<<" bytes ) into "
	    <<progress<<" bytes; average error level "<<avgsnr/i<<endl;
    	cout<<"Y_DIM: "<<(int)param.y_dim<<", encoding: "<<ttt/freq<<" ms, reading: "<<ttt3/freq
	<<" ms  ( "
	<< qual_mul*1.5*(param.y_dim+64)*(param.x_dim+64) <<" bytes in encoder, "
	<< 5*1.5*(param.y_dim+64)*(param.x_dim+64)/(ttt/freq)<<" bytes/ms, " << param.x_dim*param.y_dim*num_frames/(ttt/freq)<<" pixels/ms )"<<endl;
        cout<<"Decoding: "<<ttt2/freq<<" ms"<<endl;
        cout<<"Reading: "<<ttt3/freq<<" ms"<<endl;
        encore(handle, ENC_OPT_RELEASE, 0, 0);
//	    cout<<"Block decisions: intra "<<intra_decisions<<", inter "<<inter_decisions<<endl;
	return 0;
    }
    delete file;
}
