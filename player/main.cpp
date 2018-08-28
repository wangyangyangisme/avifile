/********************************************************

AVI player
Copyright 2000 Eugene Kuznetsov  (divx@euro.ru)

*********************************************************/

#include "playercontrol.h"

#include "version.h"
#include "avm_args.h"
#include "avm_output.h"
#include "avm_except.h"
#include "configfile.h"

#include <qapplication.h>
#include <qtextcodec.h>
#include <qtimer.h>

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif
#ifdef __NetBSD__
#include <ieeefp.h>
#endif

#include <string.h>
#include <stdlib.h> // atoi
#include <stdio.h>

PlayerControl *m;

extern void killer(int);
extern const char* g_pcProgramName;

int main(int argc, char* argv[])
{
#if defined(__FreeBSD__) || defined(__NetBSD__)
    fpsetmask(fpgetmask() & ~(FP_X_DZ|FP_X_INV));
#endif
    double testfp = argc / 1.4;
    if (GetAvifileVersion() != AVIFILE_VERSION)
    {
	fprintf(stderr, "This binary was compiled for Avifile ver. %x "
		", but the library is ver. %x.\nAborting...\n",
		GetAvifileVersion(), AVIFILE_VERSION);
	return 0;
    }

    char* acodec = 0;
    char* vcodec = 0;
    char* subname = 0;
    AviPlayerInitParams apip;
    apip.x = apip.y = 0x7fffffff;

    static const avm::Args::Option sizesubopts[] =
    {
	{ avm::Args::Option::INT, "w", "width", "window width", &apip.width, 8, 8192 },
	{ avm::Args::Option::INT, "h", "height", "window height", &apip.height, 2, 8192 },
	{ avm::Args::Option::INT, "x", 0, "window position x", &apip.x, -8192, 8192 },
	{ avm::Args::Option::INT, "y", 0, "window position y", &apip.y, -8192, 8192 },
	{ avm::Args::Option::NONE }
    };

    static const avm::Args::Option opts[] =
    {
	{ avm::Args::Option::BOOL, "a", "auto", "use autoquality for postprocessing", &apip.quiet },
	{ avm::Args::Option::SUBOPTIONS, "s", "size", "set windows size", (void*) &sizesubopts },
	{ avm::Args::Option::DOUBLE, 0, "seek", "seek to given position", &apip.seek },
	{ avm::Args::Option::CODEC, "vc", "video-codec", "preferred video codec (use help for select)", &vcodec },
	{ avm::Args::Option::CODEC, "ac", "audio-codec", "preferred audio codec (use help for select)", &acodec },
	//{ avm::Args::Option::INT, "s", "shift", "shift frame buffer by <%d> bytes [%d, %d]", &shift, -8192, 8192 },
	//{ avm::Args::Option::BOOL, NULL, "nogui", "start playing without gui", &apip.nogui },
	{ avm::Args::Option::BOOL, "f", "fullscreen", "start playing in fullscreen", &apip.fullscreen },
	{ avm::Args::Option::BOOL, "m", "maximize", "start playing in maximized", &apip.maximize },
	{ avm::Args::Option::BOOL, "q", "quiet", "be quiet", &apip.quiet },
	{ avm::Args::Option::STRING, "sub", "subtitle", "subtitle filename", &subname },
	//{ avm::Args::Option::BOOL, "t", "verbose", "be verbose", &verbose },
	//{ avm::Args::Option::BOOL, "v", "version", "show version", NULL },
	{ avm::Args::Option::CODEC, "c", "codec", "use with help for more info", 0 },
	{ avm::Args::Option::HELP },
	{ avm::Args::Option::OPTIONS, 0, 0, 0, (void*) avm::IAviPlayer::options },
	{ avm::Args::Option::NONE }
    };

    int rc=0; // remote control support over stdin/stdout
    avm::Args(opts, &argc, argv,
	      " [options] [<video-files>]\n  Options:",
	      g_pcProgramName);
#if 0
    printf("Parsed opts: left args: %d\n"
	   "Size  w:%d  h:%d  x:%d  y:%d\n"
	   "Auto  %d\n"
	   "Seek  %f\n"
	   "Fullscreen %d  Maximize %d  Quiet %d\n"
	   "VideoCodec %s  AudioCodec %s\n"
	   "Subtitle %s\n", argc,
	   apip.width, apip.height, apip.x, apip.y,
	   apip.autoq, apip.seek,
	   apip.fullscreen, apip.maximize, apip.quiet,
	   vcodec, acodec, subname
	  );
#endif
    if (acodec) { apip.acodec = acodec; free(acodec); }
    if (vcodec) { apip.vcodec = vcodec; free(vcodec); }
    if (subname) { apip.subname = subname; free(subname); }

    for (int i = 1; i < argc; i++)
    {
	if (argv[i][0] != '-')
	{
/*	    static const char* sfx[] = { ".txt", ".sub", ".smi", ".srt", 0 };
	    const char** arr = sfx;
	    while (*arr)
	    {
		int l = strlen(argv[i]);
		if (l > 4 && strncasecmp(argv[i] + l - 4, *arr, 4) == 0
                    &&
		{
		    avm::string subname(m_Filename.c_str(), m_Filename.size() - 4);
		}

	    }*/
	    apip.urls.push_back(argv[i]);
	}
    }

//    if (apip.urls.size() > 1)
//	apip.subname = apip.urls.back();

    if (apip.quiet)
	avm::out.resetDebugLevels(-1);

/*
     if (!strcmp(argv[i], "--rc") || !strcmp(argv[i], "-rc") || !strcmp(argv[i], "-r"))
	{
	    rc=1;
	    // non-buffered stdout
	    setvbuf (stdout, (char *)NULL, _IOLBF, 0);
	}
	else if (!strcmp(argv[i], "--quality") || !strcmp(argv[i], "-q"))
	{
	    if (++i >= argc)
	    {
		Usage();
		return -1;
	    }
	    if (!strcmp(argv[i], "auto") || !strcmp(argv[i], "a"))
		apip.quality = 11;
	    else
	    {
		apip.quality = atoi(argv[i]);
		printf("quality setting %d is not yet implemented...\n", use_quality);
	    }
	}
    }
*/
    QApplication a(argc, argv);
    //QApplication a(argc, argv, false);//, !apip.nogui);
    a.setDefaultCodec( QTextCodec::codecForLocale() );

    m = new PlayerControl();//, !apip.nogui);
    a.setMainWidget( m->getWidget() );

    if (apip.urls.size())
	m->initPlayer(apip);
    else
	m->openSlot();

    // signal(SIGINT, killer);
#if 0
    QTimer t;
    if(rc)
    {
        QObject::connect(&t, SIGNAL(timeout()), m, SLOT(remote_command()));
        sleep (2);
	t.start(100);
    }
#endif
    a.exec();
    delete m;
    return 0;
}
