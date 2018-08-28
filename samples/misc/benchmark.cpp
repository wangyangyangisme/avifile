/********************************************************

	Benchmarking tool
	Copyright 2000 Eugene Kuznetsov (divx@euro.ru)

*********************************************************/


#include <avifile.h>
#include <aviplay.h>
#include <avm_fourcc.h>
#include <avm_cpuinfo.h>
#include <avm_except.h>
#include <avm_creators.h>
#include <utils.h>
#include <version.h>
#include <renderer.h>

#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef X_DISPLAY_MISSING
#include <X11/Xlib.h>
#endif

#define __MODULE__ "benchmark"

const char* g_pcProgramName = "benchmark";
volatile int sig_int_seen = 0;
static void sig_int(int)
{
    sig_int_seen = 1;
}

int main(int argc, char **argv)
{
//    QApplication a(argc,argv);

    if (GetAvifileVersion()!=AVIFILE_VERSION)
    {
	printf("This binary was compiled for Avifile ver. %d, "
	       "but the library is ver. %d -  Aborting.\n",
	       AVIFILE_VERSION, GetAvifileVersion());
	return 0;
    }

#ifdef SMARTMEMCPY
    int size = 630*300*2 & ~128;
    int caches = 0*1024;
    int cnt = 1000;

    //char* a = (char*)malloc(size + 10);
    //char* b = (char*)malloc(size + 10);
    char* a = (char*)memalign(128, size + 10);
    char* b = (char*)memalign(128, size + 10);

    int64_t t1 = longcount();
    for (int i = 0; i < cnt/2; i++)
    {
	memcpy(a, b, size);
#if 0
	memcpy(a, b, size);
#else
	memcpy(a, b + size - testm, caches);
	memcpy(a, b, size - caches);
#endif
    }
    int64_t t2 = longcount();
    float d = to_float(t2, t1);
    cout << "Time " <<  d << "  " << cnt*size / d /  1024 / 1024  << " MB/s" << endl;
    cout << "HEX " << (void*) a << "   " << (void*)b << endl;
    exit(0);
#endif


    int result;
    char* fn = 0;
    avm::IVideoRenderer* renderer=0;
    int use_buffers = 4;
    int use_zoom = 0;
    int width = 0, height = 0;
    int quality;
    int qual_key;
    int status;
    unsigned int measureLength = 100;
    framepos_t framepos = 0;
    bool use_yuv = false;
    bool use_yv12 = false;
    bool hurry = false;
    bool direct = false;
    bool real_mode = false;
    bool video = true;
    bool draw = true;
    double seekTime = 0.0;

    static const avm::Args::Option sizesubopts[] =
    {
	{ avm::Args::Option::INT, "w", "width", "window width", &width, -8192, 8192 },
	{ avm::Args::Option::INT, "h", "height", "window height", &height, -8192, 8192 },
	{ avm::Args::Option::NONE }
    };

    static const avm::Args::Option opts[] =
    {
	{ avm::Args::Option::BOOL, "yuv", 0, "use hardware acceleration if it is available. Assumes -sdl", &use_yuv },
	{ avm::Args::Option::BOOL, "yv12", 0, "use YV12 hardware acceleration if it is available. Assumes -sdl.", &use_yv12 },
	{ avm::Args::Option::BOOL, "real", 0, "", &real_mode },
	{ avm::Args::Option::BOOL, "hurry", 0, "test decoder for ability to 'hurry up'", &hurry },
	{ avm::Args::Option::BOOL, "direct", 0, "use direct video memory buffers", &direct },
	{ avm::Args::Option::BOOL, "video", 0, "render video (disable with off)", &video },
	{ avm::Args::Option::BOOL, "draw", 0, "draw video (disable with off)", &draw },

	{ avm::Args::Option::SUBOPTIONS, "s", "size", "set windows size", (void*) &sizesubopts },

	{ avm::Args::Option::INT, "frame", 0, "seek to the given #frame in the movie", &framepos },
	{ avm::Args::Option::INT, "length", 0, "measure #frames (100 defauls, 0 - full movie)", &measureLength },
	{ avm::Args::Option::INT, "buffers", 0, "use # buffers (<2 - disable)", &use_buffers },

	{ avm::Args::Option::INT, "pp", 0, "postprocessing", &quality },

	{ avm::Args::Option::DOUBLE, "seek", 0, "seek to the given time in the movie", &seekTime },

	{ avm::Args::Option::HELP },
	{ avm::Args::Option::OPTIONS, 0, 0, 0, (void*) avm::IAviPlayer::options },
	{ avm::Args::Option::NONE }
    };

    avm::Args(opts, &argc, argv,
	      " [options] [<video-file>]\n"
	      "  Performs a measurement of Avifile performance for given file.\n"
	      //"  -size x y Zoom picture to dimensions x*y\n"
	      "  Description of result:\n"
	      "   \'Decompression\': Time spent on reading one video frame from file\n"
	      "                      and its decompression.\n"
	      "   \'Drawing\'      : Time spent on X server request.\n"
	      "   \'Sync\'         : Time spent by X server on drawing the frame.\n"
	      "  Options:\n",
	      g_pcProgramName);

    for (int i = 1; i < argc; i++)
	if (argv[i][0] != '-')
	{
	    fn = argv[i];
	    printf("Detected filename: %s\n", fn);
            break;
	}
/*
	else if (!strcmp(argv[i], "-size"))
	{
		use_zoom=1;
		xs=atoi(argv[i+1]);
		ys=atoi(argv[i+2]);
		if((xs<0)||(ys<0))use_zoom=0;
		}
		*/
    if (use_yv12)
        use_yuv = true;
    int64_t total_time_begin = longcount();
    double read_times=0, draw_times=0, sync_times=0, hurry_times=0;
    unsigned int frames=0;
    avm::IReadFile *file=0;
    Display* dpy=0;
    try
    {
	//char a[2023];
	file = avm::CreateReadFile(fn);
	if (!file)
	    throw FATAL("Cannot open video stream");

	avm::IReadStream *stream = file->GetStream(0, avm::IStream::Video);

	//printf("TIME  %f   %d\n", stream->GetTime(), stream->GetPos());
	if (!stream || stream->StartStreaming() != 0)
	    throw FATAL("Cannot decode this video stream");
	//printf("TIME  %f   %d\n", stream->GetTime(), stream->GetPos());

#ifndef X_DISPLAY_MISSING
	if (video)
	{
	    dpy=XOpenDisplay(0);
	    if(!dpy)throw FATAL("Cannot open X display");
	    stream->GetVideoDecoder()->SetDestFmt(avm::GetPhysicalDepth(dpy));
	}
#endif
        BITMAPINFOHEADER bh;
	stream->GetOutputFormat(&bh, sizeof(bh));
        if (seekTime)
	{
	    if (seekTime < 0 || seekTime > stream->GetLengthTime())
		//if (stream->SeekTimeToKeyFrame(seekTime) < 0)
		throw FATAL("Bad seek location");
	    stream->SeekTimeToKeyFrame(seekTime);

	    while (stream->GetTime()<seekTime)
	    {
		//cout << "GetTime " << stream->GetTime() << "  " << seekTime << endl;
		if (stream->ReadFrame(false) < 0)
		    throw FATAL("Bad seek location");
	    }
	}
	else if (framepos)
	{
	    stream->SeekToKeyFrame(framepos);
	    // throw FATAL("Bad seek location");
	    while (stream->GetPos()<framepos)
		if (stream->ReadFrame(false) != 0)
		    throw FATAL("Bad seek location");
	}
	bh.biHeight = labs(bh.biHeight);
	printf("Movie size: %dx%d  [%.4s]\n", bh.biWidth, bh.biHeight, (char*) &bh.biCompression);

	if (measureLength && (stream->GetLength() - stream->GetPos()) < measureLength)
	    throw FATAL("Movie too short for given length");

	try
	{
	    if (!use_yuv)
		throw FATAL("YUV renderer not requested");
	    BITMAPINFOHEADER bhy;
	    stream->GetVideoFormat(&bhy, sizeof(bhy));
	    fourcc_t fcc;
	    avm::IVideoDecoder::CAPS caps = stream->GetVideoDecoder()->GetCapabilities();
	    printf("Decoder YUV capabilities: 0%x\n", caps);
	    if (caps & avm::IVideoDecoder::CAP_YUY2)
		fcc = fccYUY2;
	    else if (caps & avm::IVideoDecoder::CAP_YV12)
		fcc = fccYV12;
	    else if (caps & avm::IVideoDecoder::CAP_UYVY)
		fcc = fccUYVY;
	    else
		throw FATAL("YUV format unsupported by decoder");

	    if (use_yv12 && caps & avm::IVideoDecoder::CAP_YV12)
		fcc = fccYV12;

#ifndef X_DISPLAY_MISSING
            if (video)
		renderer = avm::CreateYUVRenderer(0, dpy, bhy.biWidth, bhy.biHeight, fcc);
#endif

	    if (fcc)
	    {
		if (stream->GetVideoDecoder()->SetDestFmt(0, fcc))
		    //shouldn't happen
		    throw FATAL("Error setting YUV decoder output");
	    }
	}
	catch (FatalError& e)
    	{
	    e.Print();
	    delete renderer;
#ifndef X_DISPLAY_MISSING
            if (video)
		renderer = avm::CreateFullscreenRenderer(0, dpy, bh.biWidth, bh.biHeight);
#endif
	}
//        m->move(100,100);
//        m->resize(20,20);

	if (width > 0 && height > 0 && renderer)
	    renderer->Resize(width, height);
	else
	{
	    width = bh.biWidth;
	    height = bh.biHeight;
	}

	stream->SetBuffering(use_buffers, direct ? renderer : 0);

        frames = 0;
	int read_counts = 0, hurry_counts = 0;

	sig_t old_int = signal(SIGINT, sig_int);

	while (!sig_int_seen
	       && !stream->Eof()
	       && (!measureLength || frames < measureLength))
	{
	    int64_t t1 = longcount();

	    //printf("TIME  %f   %d\n", stream->GetTime(), stream->GetPos());
	    stream->ReadFrame(hurry?(frames%5?false:true):draw);
	    //stream->ReadFrame(hurry?(frames%5?false:true):true);
	    //cout << "GetPos " << stream->GetPos() << endl;
	    avm::CImage* im = stream->GetFrame();
	    if (im)
	    {
		int64_t t2 = longcount();
		if (renderer) renderer->Draw(im);
		int64_t t3 = longcount();
		if (renderer) renderer->Sync();
		int64_t t4 = longcount();
                im->Release();
		if (hurry)
		{
		    if (frames % 5)
		    {
			hurry_counts++;
			hurry_times += to_float(t2,t1);
		    }
		    else
		    {
			read_counts++;
			read_times += to_float(t2,t1);
		    }
		}
		else
		{
		    read_times += to_float(t2,t1);
		    read_counts++;
		}
		draw_times += to_float(t3,t2);
		sync_times += to_float(t4,t3);
		frames++;
	    }
	    else
		printf("Zero image! - benchmark invalid\n");
	}
	if (sig_int_seen)
            printf("*** BREAK ***\n");
	signal(SIGINT, old_int);
	float total_time = to_float(longcount(), total_time_begin);
	printf("Played %d frames in %.3f  (t: %.3f) seconds ( avg frame rate %.3f fps )\n",
	       frames, (hurry_times+read_times+draw_times+sync_times), total_time,
	       frames/(read_times+draw_times+sync_times+hurry_times));
	if (frames)
	{
	    if (!hurry)
		printf("Average (Total) results:\n"
		       "\tDecompression %f ms\t(%.3fs)\n"
		       "\tDrawing %f ms\t\t(%.3fs)\n"
		       "\tSync %f ms\t\t(%.3fs)\n",
		       read_times/frames * 1000., read_times,
		       draw_times/frames * 1000., draw_times,
		       sync_times/frames * 1000., sync_times);
	    else
		printf("Average results:\n\t"
		       " Hurry decompression %f ms\n\t"
		       " Normal decompression %f ms\n\t"
		       " Drawing %f ms\n\t"
		       " Sync %f ms\n",
		       hurry_times/hurry_counts * 1000.,
		       read_times/read_counts * 1000.,
		       draw_times/frames * 1000., sync_times/frames * 1000.);
	}
#ifndef X_DISPLAY_MISSING
	if (!use_yuv && sync_times > 0)
	{
	    printf("Average video output speed: %f Mb/s\n",
		   avm::GetPhysicalDepth(dpy)/8*width*height*frames/sync_times/(1024*1024));
	}
#endif
    }
    catch (FatalError& error)
    {
	error.Print();
    }
    delete file;
    delete renderer;
#ifndef X_DISPLAY_MISSING
    if (dpy)
	XCloseDisplay(dpy);
#endif
    //delete m;
    return 0;
}
