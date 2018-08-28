
#include <qapplication.h>
#include <qwidget.h>
#include <qpaintdevice.h>

#include "qtrenderer.h"

#include <renderer.h>
#include <mmx.h>
#include <avm_except.h>

//#define QT_CLEAN_NAMESPACE

#include <unistd.h>
#ifdef __FreeBSD__
#include <machine/param.h>
#include <sys/types.h>
#endif
#include <sys/ipc.h>
#include <sys/shm.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#define __MODULE__ "Shm renderer"
inline static void qSafeXDestroyImage( XImage *x )
{
    if ( x->data ) {
	free( x->data );
	x->data = 0;
    }
    XDestroyImage( x );
}

ShmRenderer::ShmRenderer(QWidget* w, int x, int y, int _xpos, int _ypos)
: xshmimg(0), xshmpm(0), xpos(_xpos), ypos(_ypos), m_w(x), m_h(y),
  pic_w(x), pic_h(y), dev((QPaintDevice*)w), xshminit(false)
{
    alloc();
}

ShmRenderer::~ShmRenderer()
{
    free();
}

void ShmRenderer::alloc()
{
    int major, minor;
    Bool pixmaps_ok;
    Display *dpy = dev->x11Display();
    int dd	 = dev->x11Depth();
    Visual *vis	 = (Visual*)dev->x11Visual();
    //	printf("Creating SHM renderer, width %d, height %d\n", pic_w, pic_h);
    //	printf("Server vendor %s, release %d\n", ServerVendor(dpy), VendorRelease(dpy));

    try
    {
	XGCValues xcg;
	xcg.graphics_exposures=false;
	gc=XCreateGC(dpy, dev->handle(), GCGraphicsExposures, &xcg);
	if ( !XShmQueryVersion(dpy, &major, &minor, &pixmaps_ok) )
	    throw FATAL("MIT SHM extension not supported");

	bool ok;
	int _pic_w=((pic_w+m_w-1)/m_w)*m_w;
	int _pic_h=((pic_h+m_h-1)/m_h)*m_h;
	xshminfo.shmid = shmget( IPC_PRIVATE, _pic_w*_pic_h*4,
				 IPC_CREAT | 0777 );
	ok = xshminfo.shmid != -1;
	if(!ok)
	    throw FATAL("Can't get shared memory segment");

	xshminfo.shmaddr = (char*)shmat( xshminfo.shmid, 0, 0 );
	ok = xshminfo.shmaddr != 0;
	xshminfo.readOnly = FALSE;

	if ( !ok )
	    throw FATAL("Can't attach shared memory segment");

	ok = XShmAttach( dpy, &xshminfo );
	if ( !ok )
	    throw FATAL("XShmAttach failed");

	xshmimg = XShmCreateImage( dpy, vis, dd, ZPixmap, xshminfo.shmaddr, &xshminfo, _pic_w, _pic_h );

	if ( !xshmimg )
	    throw FATAL("Can't create shared image");
    }
    catch(...)
    {
	xshmimg = 0;
	if ( xshminfo.shmaddr )
	    shmdt( xshminfo.shmaddr );
	if ( xshminfo.shmid != -1 )
	    shmctl( xshminfo.shmid, IPC_RMID, 0 );
	throw;
    }
}
int ShmRenderer::free()
{
    printf("Free()\n");
    if ( xshmimg == 0 )
	return 0;
    Display *dpy = dev->x11Display();
    XSync(dpy, false);
    if ( xshmpm ) {
	XFreePixmap( dpy, xshmpm );
	xshmpm = 0;
    }
    XShmDetach( dpy, &xshminfo ); xshmimg->data = 0;
    qSafeXDestroyImage( xshmimg ); xshmimg = 0;
    shmdt( xshminfo.shmaddr );
    shmctl( xshminfo.shmid, IPC_RMID, 0 );
    XFreeGC(dpy, gc);

    xshminfo.shmaddr=0;
    xshminfo.shmid=0;
    return 0;
}

int ShmRenderer::resize(int& new_w, int& new_h)
{
    if(new_w<0)return -1;
    if(new_h<0)return -1;
    new_w&=(~7);
    new_h&=(~7);
    int xratio=(new_w+m_w-1)/m_w;
    int yratio=(new_h+m_h-1)/m_h;
    int old_xratio=(pic_w+m_w-1)/m_w;
    int old_yratio=(pic_h+m_h-1)/m_h;
    printf("New size: %d %d\n", new_w, new_h);
    pic_w=new_w;
    pic_h=new_h;
    if((xratio!=old_xratio)||(yratio!=old_yratio))
    {
	mutex.Lock();
	free();
	alloc();
	mutex.Unlock();
    }
    return 0;
}

static void copy_deinterlace_24(void* outpic, const void* inpic, int xdim, int height)
{
#ifdef ARCH_X86
    for(int i=0; i<height; i++)
    {
	char* outp=((char*)outpic)+i*xdim;
	const char* inp=((const char*)inpic)+i*xdim;
	if((i==0) || (i==height-1))
	{
	    memcpy(outp, inp, xdim);
	    continue;
	}
	int count=xdim/8;
	__asm__ __volatile__ (
	"movl %2, %%esi\n\t"
	"movl %3, %%edi\n\t"
	"pxor %%mm3, %%mm3\n\t"
	"1:\n\t"
	"movq (%%ecx, %%esi), %%mm0\n\t"
	"movq (%%ecx), %%mm1\n\t"
	"movq %%mm1, %%mm2\n\t"
	"pavgb %%mm0, %%mm2\n\t"
	"psadbw %%mm2, %%mm3\n\t"
	"movd %%mm3, %%eax\n\t"
	"cmpl $48, %%eax\n\t"
	"jb 2f\n\t"
	"pavgb %%mm1, %%mm0\n\t"
	"2:\n\t"
	"movq %%mm0, (%%edx)\n\t"
	"addl $8, %%ecx\n\t"
	"addl $8, %%edx\n\t"
	"decl %%edi\n\t"
	"jnz 1b\n\t"
    	:
	: "c" (inp-xdim), "d" (outp), "r" (xdim), "r" (count)
	: "esi", "edi"
	);
    }
    __asm__ __volatile__ ("emms\n\t");
#else
#warning missing copy_deinterlace_24
#endif
}

void ShmRenderer::move(int x, int y)
{
    xpos = x;
    ypos = y;
}

int ShmRenderer::draw(QPainter* pm, const void* data, bool deinterlace)
{
    char* outpic=xshmimg->data;
    if(outpic==0)
	return 0;
    if(data==0)
	return 0;

    Display *dpy = dev->x11Display();
    Visual *vis = (Visual*)dev->x11Visual();

    int bit_depth = avm::GetPhysicalDepth(dpy);

    if (mutex.TryLock() != 0)
	return -1;
    //	pthread_mutex_lock(&mutex);

    sync();
    const char* src;

    //	if((pic_h==m_h)	&& (pic_w==m_w))
    if(!deinterlace)
	memcpy(outpic, data, m_w*m_h*((bit_depth+7)/8));
    else
	switch(bit_depth)
	{
	    //	    case 15:
	    //		copy_deinterlace_555(outpic, data, m_w, m_h);
	    //		break;
	    //	    case 16:
	    //		copy_deinterlace_565(outpic, data, m_w, m_h);
	    //		break;
	case 24:
	case 32:
	    copy_deinterlace_24(outpic, data, m_w*bit_depth/8, m_h);
	    break;
	default:
	    memcpy(outpic, data, m_w*m_h*((bit_depth+7)/8));
	    break;
	}
    //	else
    //	{
    //	    printf("Zooming\n");
    //	    zoom((unsigned short*)outpic, (unsigned short*)data, pic_w, pic_h, m_w, m_h, bit_depth, m_w*((pic_w+m_w-1)/m_w));
    //	}

    XShmPutImage(dpy, dev->handle(), gc,
		 xshmimg, 0, 0, xpos, ypos, pic_w, pic_h, true);

    mutex.Unlock();

    return 0;
}

int ShmRenderer::sync()
{
    Display *dpy = dev->x11Display();
    XSync(dpy, false);
    return 0;
}
