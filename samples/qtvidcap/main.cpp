
#include "v4lwindow.h"
#include "vidconf.h"

#include <aviplay.h>
#include "v4lxif.h"
#include <avm_except.h>
#include <avm_cpuinfo.h>
#include <videoencoder.h>
#include <VideoDPMS.h>
#include <version.h>
#define DECLARE_REGISTRY_SHORTCUT
#include <configfile.h>
#undef DECLARE_REGISTRY_SHORTCUT

#include <qapplication.h>
#include <qtextcodec.h>

#ifdef HAVE_SYSINFO
#include <sys/sysinfo.h>  // is this standard for all linux boxes ?
#endif
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>


unsigned int m_iMemory;
const char* g_pcProgramName = "AviCap";

static v4lxif* createDevice()
{
    v4lxif* v4l = 0;
    try
    {
	v4l = new v4l1if(0, RS("CaptureDevice", "/dev/video"));
        return v4l;
    }
    catch(FatalError& e)
    {
	e.PrintAll();
	delete v4l;
    }
    
    return 0;
}

#if 0
// We do not need this ugly CPU hungry busy loop hack anymore
// kabi@users.sf.net
class QMyApp: public QApplication
{
    V4LWindow* m_pWin;
public:
    QMyApp(int argc, char** argv): QApplication(argc, argv), m_pWin(0){}
    void setWindow(V4LWindow* pWin) { m_pWin=pWin; }
    virtual bool     x11EventFilter( XEvent * e )
    {
	if(m_pWin)
	    m_pWin->get_clips();
	return QApplication::x11EventFilter(e);
    }
};
#endif

static const QString trans_path[] = { ".", I18N_PATH, "" };

int main(int argc, char** argv)
{
//    Registry::ReadString("QtVidcap", "CapDev", "/dev/video");
    QApplication a(argc,argv);
#if 0
    QTranslator qtrans( 0 );
    qtrans.load( QString( "qt_" ) + QTextCodec::locale(), "." );
    a.installTranslator( &qtrans );
#endif

    // translation file for application strings
    QTranslator mytrans( 0 );
    int i=0;
    bool res=false;
    do{
      res=mytrans.load( QString( "avicap_" ) + QTextCodec::locale(), trans_path[i] );
      if(res){
	printf("translation loaded from %s\n",trans_path[i].latin1());
      }
      i++;
    }while(!(res || trans_path[i]==""));

    a.installTranslator( &mytrans );


    if (GetAvifileVersion()!=AVIFILE_VERSION)
    {
	printf("This binary was compiled for Avifile ver. %x, "
	       "but the library is ver. %x. Aborting.\n",
	       AVIFILE_VERSION, GetAvifileVersion());
	return 0;
    }
    bool bNow=false;
    bool bTimer=false;
    if(argc==2){
      if(!strcmp(argv[1], "-now")){
	bNow=true;
      }
      else if(!strcmp(argv[1], "-timer")){
	bTimer=true;
      }
    }
    avm::string cmd = avm::string("kv4lsetup -t=2 -l ") + RS("CapDev", "/dev/video");

#ifdef AVICAP_PARANOID
      sleep(10);
#endif

    if ((cmd.find(':')!=avm::string::npos)
	|| (cmd.find('|')!=avm::string::npos)
	|| (cmd.find(';')!=avm::string::npos))
	{
	    system("kv4lsetup -t=2");
//	    printf("Retval: %d errno: %d\n", val, errno);
	}
    else
    {
	system(cmd.c_str());
//	printf("str: %s\n", cmd.c_str());
//        printf("Retval: %d errno: %d\n", val, errno);
    }


#ifdef AVICAP_PARANOID
      sleep(10);
#endif

#ifdef HAVE_SYSINFO
    struct sysinfo s_info;
    sysinfo(&s_info);
    m_iMemory=s_info.totalram;
    if(m_iMemory<2*1048576)
	m_iMemory=1048576;
    else
	m_iMemory/=2;
    printf("Using %dMb of memory for frame caching\n", m_iMemory/1048576);
#else
    m_iMemory = 16 * 1024 * 1024;
#endif    
    int result;

    int p = getpriority(PRIO_PROCESS, 0);
    //attention: only root is allowed to lower priority
    setpriority(PRIO_PROCESS, 0, (p + 3 < 20) ? p + 3 : 20);

    while(1)
    {    
	v4lxif* pV4L = createDevice();
#ifdef AVICAP_PARANOID
	sleep(10);
#endif
	if (pV4L)
	{
	    V4LWindow m(pV4L);
	    avm::VideoDPMS dp(m.x11Display());
	    if (bNow)
		m.setAutoRecord();
	    m.show();
	    m.setupDevice();
	    //a.setWindow(&m);
	    if(bTimer){
	      m.startTimer();
	    }
	    a.setMainWidget(&m);
	    a.exec();
	    return 0;
	}
	else
	{
	    VidConfig conf(0, pV4L, 0);
	    if (conf.exec()==QDialog::Rejected)
		return -1;
	}
    }
}
