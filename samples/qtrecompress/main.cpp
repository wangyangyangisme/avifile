#include "genctl.h"
#include "recwnd.h"

#include <avm_except.h>

#include <qapplication.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif
#ifdef __NetBSD__
#include <ieeefp.h>
#endif

void Usage()
{
    printf("Usage: avirecompress [config-file-name]\n"
	   "   Without options: interactive mode\n"
	   "   With one option: batch mode ( processes one file according to config options and exits )\n");
}

struct ConsoleCallback: public IRecompressCallback
{
    int state;
    ConsoleCallback():state(0){}
    virtual void setNewState(double progress, double elapsed, int64_t filesize)
    {
	printf("percent: %3.1f  elapsed: %f   size: %lld\n", progress * 100, elapsed, filesize);
    }
    virtual void setTotal(framepos_t vtotal, framepos_t atotal,
			  double vtime, double atime) {}
    virtual void addVideo(framepos_t vframe, unsigned int vsize, double vtime, bool keyframe) {}
    virtual void addAudio(framepos_t asample, unsigned int asize, double atime) {}
    virtual void finished() { state=1; }
};

int main(int argc, char** argv)
{
#if defined(__FreeBSD__) || defined(__NetBSD__)
    fpsetmask(fpgetmask() & ~(FP_X_DZ|FP_X_INV));
#endif
    if (argc == 1)
    {
        try
	{
	    QApplication app(argc, argv);
    	    QtRecompressorCtl ctl;
	    ctl.show();
	    app.setMainWidget(&ctl);
	    app.exec();
	}
        catch (FatalError& e)
	{
	    e.PrintAll();
        }
    }
    else if (argc == 2)
    {
	if (strcmp(argv[1], "--help")!=0)
	{
	    ConsoleCallback ck;
	    RecKernel kernel;
	    kernel.loadConfig(argv[1]);
	    kernel.setCallback(&ck);
	    kernel.startRecompress();
	    while (!ck.state)
		sleep(1);
//	    RecWindow* wnd=new RecWindow(0, &kernel);
//	    wnd->show();
//	    app.setMainWidget(wnd);
//	    wnd->exec();
	}
	else
	    Usage();
    }
    else
	Usage();
    return 0;
}
