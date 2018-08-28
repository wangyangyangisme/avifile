/*
 kv4lSetup using Bt848 frame grabber driver

 Copyright (C) 1998 Moritz Wenk (wenk@mathematik.uni-kl.de)
 Original v4l-setup (C) by Gerd Knorr <kraxel@cs.tu-berlin.de>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// need this to avoid conflicting type declarations in Xmd.h and qglobal.h
//#define QT_CLEAN_NAMESPACE

#include <config.h>
#include <avm_args.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#include <asm/types.h>          /* XXX glibc */

/* Necessary to prevent collisions between <linux/time.h> and <sys/time.h> when V4L2 is installed. */
#define _LINUX_TIME_H
#include <linux/videodev.h>

#ifndef X_DISPLAY_MISSING
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_LIBXXF86DGA
#include <X11/extensions/xf86dga.h>
#endif

#ifdef HAVE_LIBXXF86VM
#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/xf86vmstr.h>
#endif

#endif /* X_DISPLAY_MISSING */

#ifndef major
#define major(dev)  (((dev) >> 8) & 0xff)
#endif

#define V4L_DEVICE "/dev/video"


static int dev_open(const char *device, int major)
{
    int		fd;
#if 0
    if (!strncmp(device, "/dev/", 5) || strchr(device + 5, '/')) {
	fprintf(stderr, "warning: %s is not a /dev file\n", device);
	exit(1);
    }
#endif

    /* open & check v4l device */
    fd = open(device,O_RDWR);
    if (fd != -1)
    {
	struct stat stb;
	if (fstat(fd,&stb) != -1)
	{
	    if (S_ISCHR(stb.st_mode) && (major(stb.st_rdev) == major))
		return fd;
            else
		fprintf(stderr, "%s: wrong device\n", device);
	}
        else
	    fprintf(stderr, "fstat(%s): %s\n", device, strerror(errno));
        close(fd);
    }
    else
	fprintf(stderr, "can't open %s: %s\n", device, strerror(errno));

    exit(1);
}

int main(int argc, char* argv[])
{
    int bpp = 0;
    int shift = 0;
    int verbose = 0;
    int bequiet = false;
    const char* display = ":0.0";
    const char* device = V4L_DEVICE;
    char* h;
    int depth;
    struct video_capability capability;
    struct video_buffer fbuf;
    int fd, c;
    int i, n, v, found;
    int set_width, set_height, set_bpp, set_bpl;
    void* set_base = NULL;

#ifndef X_DISPLAY_MISSING
    Display* dpy;
    Screen* scr;
    Window root;
    XVisualInfo* info, xvtemplate;
    XWindowAttributes wts;
    XPixmapFormatValues* pf;
#ifdef HAVE_LIBXXF86DGA
    int width,bar,foo,flags,ma,mi;
    void* base = 0;
#endif
#ifdef HAVE_LIBXXF86VM
    int vm_count;
    XF86VidModeModeInfo** vm_modelines;
#endif
#endif /* X_DISPLAY_MISSING */

    h = getenv("DISPLAY");
    if (h != NULL) display = h;

    static const avm::Args::Option opts[] =
    {
	{ avm::Args::Option::INT, "b", "bpp", "color depth of the display is <%d>", &bpp, 0, 32 },
	{ avm::Args::Option::INT, "p", "palette", "set palette of display (see help for details)" },
	{ avm::Args::Option::STRING, "l", "device", "use <%s> as video4linux device", &device },
	{ avm::Args::Option::STRING, "d", "display", "use X11 display <%s>", &display },
	{ avm::Args::Option::INT, "s", "shift", "shift frame buffer by <%d> bytes [%d, %d]", &shift, -8192, 8192 },
	{ avm::Args::Option::BOOL, "q", "quiet", "be quiet", &bequiet },
	{ avm::Args::Option::INT, "t", "verbose", "be verbose", &verbose },
	{ avm::Args::Option::BOOL, "v", "version", "show version", NULL },
	{ avm::Args::Option::HELP },
	{ avm::Args::Option::CODEC, "c", "codec", "use with help for more info" },
	{ avm::Args::Option::NONE }
    };

    avm::Args(opts, &argc, argv, "[ options ]\n"
	      "  Helper tool to figure out video mode and\n"
	      "  configures a video4linux driver like bttv");

/*    verbose = pcl.getSwitch('q', false) ? 0 : 1;
    verbose = pcl.getSwitch('v', false) ? 2 : verbose;
    shift = pcl.getInt('s', shift, -8192, 8192);
 */
    // check display
    if (display[0] != ':') {
	const char* od = display;
	display = strchr(display,':');
	if (!display)
	    exit(1);
	fprintf(stderr,"non-local display `%s' not allowed, using `%s' instead\n",od,display);
    }

    // open display
#ifndef X_DISPLAY_MISSING
    dpy = XOpenDisplay(display);
    if (dpy == NULL) {
	if (verbose) fprintf(stderr,"can't open display %s\n",display);
	exit(1);
    }

    if (verbose)
	fprintf(stderr,"using X11 display %s\n",display);

    // and get params
    scr  = DefaultScreenOfDisplay(dpy);
    root = DefaultRootWindow(dpy);
    XGetWindowAttributes(dpy, root, &wts);

    if (verbose)
	fprintf(stderr,"x11: mode=%dx%dx%d\n",wts.width,wts.height,wts.depth);

    /* look for a usable visual */
    xvtemplate.screen = XDefaultScreen(dpy);
    info = XGetVisualInfo(dpy, VisualScreenMask, &xvtemplate, &found);
    /*
     v = -1;
     for (i = 0; v == -1 && i < found; i++)
     if (info[i].class == TrueColor && info[i].depth >= 15)
     v = i;
     for (i = 0; v == -1 && i < found; i++)
     if (info[i].class == StaticGray && info[i].depth == 8)
     v = i;
     if (-1 == v) {
     fprintf(stderr,"no approximate visual available\n");
     exit(1);
     }
     */

    depth = 0;
    pf = XListPixmapFormats(dpy,&n);
    for (i = 0; i < n; i++) {
	for ( v= 0; v < found; v++ ) {
	    if (pf[i].depth == info[v].depth) {
		depth = pf[i].bits_per_pixel;
		break;
	    }
	}
    }
    if (0 == depth && 0 == bpp) {
	fprintf(stderr,"can't autodetect framebuffer depth\n");
	exit(1);
    }

    if (verbose)
	fprintf(stderr,"x11: detected framebuffer depth: %d bpp\n",depth);

    if ( bpp == 8 || bpp == 15 || bpp == 16 || bpp == 24 || bpp == 32 )
	depth = bpp;
    set_bpp = (depth+7) & 0xf8;

    if (verbose)
	fprintf(stderr,"x11: set depth to %d bpp\n", depth);

#ifdef HAVE_LIBXXF86DGA
    if (XF86DGAQueryExtension(dpy, &foo, &bar)) {
	XF86DGAQueryDirectVideo(dpy, XDefaultScreen(dpy),&flags);
	if (flags & XF86DGADirectPresent) {
	    XF86DGAGetVideoLL(dpy,XDefaultScreen(dpy),(int*)&base,&width,&foo,&bar);
	    set_bpl  = width * set_bpp/8;
	    set_base = base;
	    if (verbose == 2) {
		XF86DGAQueryVersion(dpy,&ma,&mi);
		fprintf(stderr,"x11: X-Server supports DGA extention (version %d.%d)\n",ma,mi);
	    }
	    if (verbose)
		fprintf(stderr,"dga: base=%p, width=%d\n",base, width);
	}
    }
#else
    if (verbose)
	fprintf(stderr,"no dga available...\n");
#endif

#ifdef HAVE_LIBXXF86VM
    if (verbose==2) {
	foo=bar=0;
	if (XF86VidModeQueryExtension(dpy,&foo,&bar)) {
	    XF86VidModeQueryVersion(dpy,&ma,&mi);
	    fprintf(stderr,"x11: X-Server supports VidMode extention (version %d.%d)\n",
		    ma,mi);
	    if (ma == 0 && mi < 8) {
		fprintf(stderr,"x11:   VidMode v0.8 or newer required\n");
	    } else if ((ma != XF86VIDMODE_MAJOR_VERSION) || (mi != XF86VIDMODE_MINOR_VERSION)) {
		fprintf(stderr,"main: VidMode server extention version mismatch, disabled");
		fprintf(stderr,"main: server version %d.%d != included version %d.%d",ma,mi,
			XF86VIDMODE_MAJOR_VERSION,XF86VIDMODE_MINOR_VERSION);
	    } else {
		vm_modelines=(XF86VidModeModeInfo **) malloc( sizeof( XF86VidModeModeInfo * ) );
		fprintf(stderr,"\t available video mode(s):");
		XF86VidModeGetAllModeLines(dpy,XDefaultScreen(dpy),
					   &vm_count,&vm_modelines);
		fprintf(stderr," %d ",vm_count);
		for (i = 0; i < vm_count; i++) {
		    fprintf(stderr," %dx%d",
			    vm_modelines[i]->hdisplay,
			    vm_modelines[i]->vdisplay);
		}
		fprintf(stderr,"\n");
	    }
	}
    }
#endif

    set_width  = wts.width;
    set_height = wts.height;
#else
    set_width  = 384;
    set_height = 288;
#endif /* X_DISPLAY_MISSING */

    set_bpl    = set_width * set_bpp / 8;
    /* Open device file, with security checks */
    fd = dev_open(device, 81 /* VIDEO_MAJOR */);
    if (-1 == ioctl(fd, VIDIOCGCAP, &capability)) {
	fprintf(stderr,"%s: ioctl VIDIOCGCAP: %s\n",device,strerror(errno));
	exit(1);
    }

    if (-1 == ioctl(fd, VIDIOCGCAP, &capability)) {
	fprintf(stderr,"%s: ioctl VIDIOC_G_CAP: %s\n",device,strerror(errno));
	exit(1);
    }
    if (!(capability.type & VID_TYPE_OVERLAY)) {
	fprintf(stderr,"%s: no overlay support\n",device);
	exit(1);
    }

    /* read-modify-write v4l screen parameters */
    if (ioctl(fd, VIDIOCGFBUF, &fbuf) == -1) {
	fprintf(stderr,"%s: ioctl VIDIOC_G_FBUF: %s\n",device,strerror(errno));
	exit(1);
    }

    /* set values */
    fbuf.width        = set_width;
    fbuf.height       = set_height;
    fbuf.depth        = set_bpp;
    fbuf.bytesperline = set_bpl;
    if (set_base != NULL)
	fbuf.base     = (void*)((char*)set_base + shift);

    /* XXX bttv confuses color depth and bits/pixel */
#ifndef X_DISPLAY_MISSING
    if (wts.depth == 15)
	fbuf.depth = 15;
    if ((bpp == 15 || bpp == 16) && (depth == 16))
	fbuf.depth = bpp;
#endif /* X_DISPLAY_MISSING */
    if (verbose) {
	fprintf(stderr,"set video mode: %dx%d, %d bit/pixel, %d byte/scanline\n",
		fbuf.width,fbuf.height,fbuf.depth,fbuf.bytesperline);
	if (set_base != NULL)
	    fprintf(stderr,"set framebuffer at %p\n",set_base);
    }

    if (ioctl(fd,VIDIOCSFBUF,&fbuf) == -1) {
	fprintf(stderr,"%s: ioctl VIDIOC_S_FBUF: %s\n",device,strerror(errno));
	fprintf(stderr,"error: framebuffer address could not be set\n");
	fprintf(stderr,"       kv4lsetup needs root permissions\n");
	fprintf(stderr,"       kv4lsetup not correctly installed\n");
	exit(1);
    }
    if (verbose)
	fprintf(stderr,"ok\n");

    close(fd);

    return 0;
}
