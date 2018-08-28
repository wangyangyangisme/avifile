/*
 video4linux interface wrapper for kwintv

 Copyright (C) 1998,1999 Moritz Wenk (wenk@mathematik.uni-kl.de)

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

/* $Id: v4lxif.cpp,v 1.31 2005/03/23 16:16:09 kabi Exp $ */

#include "v4lxif.h"

#include <avm_except.h>
#include <avm_output.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
//#include <asm/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef BTTV_VERSION
#define BTTV_VERSION _IOR('v', BASE_VIDIOCPRIVATE+6, int)
#endif

#include <stdarg.h>

#ifndef X_DISPLAY_MISSING
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include "qt_visual.h"

#ifdef HAVE_LIBXXF86DGA
#include <X11/Xproto.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86dgastr.h>

// Support deprecated XFree86 symbol names
#ifndef XDGA_MINOR_VERSION
#define XDGA_MINOR_VERSION XF86DGA_MINOR_VERSION
#endif
#ifndef XDGA_MAJOR_VERSION
#define XDGA_MAJOR_VERSION XF86DGA_MAJOR_VERSION
#endif

#endif /* HAVE_LIBXXF86DGA */

#endif /* X_DISPLAY_MISSING */

/*                            PAL  NTSC SECAM */
static const int maxwidth[]  = { 768, 640, 768 };
static const int maxheight[] = { 576, 480, 576 };

static const int one  = 1;
static const int zero = 0;

static const struct STRTAB norms[] = {
    {  0, "PAL" },
    {  1, "NTSC" },
    {  2, "SECAM" },
    {  3, "AUTO" },
    { -1, NULL }
};

static const struct STRTAB ifname[] = {
    {  0, "no interface"  },
    {  1, "Video4Linux"  },
    {  2, "Video4Linux 2"  },
    { -1, NULL },
};

#ifdef v4lDEBUG
static const struct STRSTRTAB {
    int nr;
    const char* str;
    const char* description;
} device_cap[] = {
    { VID_TYPE_CAPTURE, "capture","Can capture to memory" },
    { VID_TYPE_TUNER, "tuner","Has a tuner of some form" },
    { VID_TYPE_TELETEXT, "teletext","Has teletext capability" },
    { VID_TYPE_OVERLAY, "overlay", "Can overlay its image onto the frame buffer" },
    { VID_TYPE_CHROMAKEY, "chromakey", "Overlay is Chromakeyed" },
    { VID_TYPE_CLIPPING, "clipping", "Overlay clipping is supported" },
    { VID_TYPE_FRAMERAM, "frameram","Overlay overwrites frame buffer memory" },
    { VID_TYPE_SCALES, "scales", "The hardware supports image scaling" },
    { VID_TYPE_MONOCHROME, "monochrome", "Image capture is grey scale only" },
    { VID_TYPE_SUBCAPTURE, "subcapture", "Capture can be of only part of the image" },
    { -1, NULL, NULL }
};
#endif

static const char* unknown = "???";
static const char* audiodescr[] = {
    "autodetection", "mono", "stereo", unknown,
    "language 1", unknown, unknown, unknown, "language 2"
};

static const char *device_pal[] = {
    "-", "grey", "hi240", "rgb16", "rgb24", "rgb32", "rgb15",
    "yuv422", "yuyv", "uyvy", "yuv420", "yuv411", "raw",
    "yuv422p", "yuv411p", NULL
};


//#define v4lDEBUG
#define VIF_FATAL(a) avml(AVML_WARN, a)
#define VIF_WARN(a)  avml(AVML_WARN, a)

static const unsigned short format2palette[] = {
    0,				/* unused */
    VIDEO_PALETTE_HI240,	/* RGB8   */
    VIDEO_PALETTE_GREY,		/* GRAY8  */
#if __BYTE_ORDER == __BIG_ENDIAN
    0,
    0,
    VIDEO_PALETTE_RGB555,	/* RGB15_BE  */
    VIDEO_PALETTE_RGB565,	/* RGB16_BE  */
    0,
    0,
    VIDEO_PALETTE_RGB24,	/* RGB24     */
    VIDEO_PALETTE_RGB32,	/* RGB32     */
#else // little endian (intel)
    VIDEO_PALETTE_RGB555,	/* RGB15_LE  */
    VIDEO_PALETTE_RGB565,	/* RGB16_LE  */
    0,
    0,
    VIDEO_PALETTE_RGB24,	/* BGR24     */
    VIDEO_PALETTE_RGB32,	/* BGR32     */
    0,
    0,
#endif
};

#define VIDEO_RGB08          1  /* bt848 dithered */
#define VIDEO_GRAY           2
#define VIDEO_RGB15_LE       3  /* 15 bpp little endian */
#define VIDEO_RGB16_LE       4  /* 16 bpp little endian */
#define VIDEO_RGB15_BE       5  /* 15 bpp big endian */
#define VIDEO_RGB16_BE       6  /* 16 bpp big endian */
#define VIDEO_BGR24          7  /* bgrbgrbgrbgr (LE) */
#define VIDEO_BGR32          8  /* bgr-bgr-bgr- (LE) */
#define VIDEO_RGB24          9  /* rgbrgbrgbrgb (BE)*/
#define VIDEO_RGB32         10  /* -rgb-rgb-rgb (BE)*/
#define VIDEO_LUT2          11  /* lookup-table 2 byte depth */
#define VIDEO_LUT4          12  /* lookup-table 4 byte depth */
#define VIDEO_YUYV	    13  /* YUV 4:2:2 */
#define VIDEO_YUV420	    14  /* YUV 4:2:0 */

static const unsigned int format2depth[] = {
    0,               /* unused   */
    8,               /* RGB8     */
    8,               /* GRAY8    */
    16,              /* RGB15 LE */
    16,              /* RGB16 LE */
    16,              /* RGB15 BE */
    16,              /* RGB16 BE */
    24,              /* BGR24    */
    32,              /* BGR32    */
    24,              /* RGB24    */
    32,              /* RGB32    */
    16,              /* LUT2     */
    32,              /* LUT4     */
    16,		     /* YUV422   */
    12,		     /* YUV420   */
};

static const char* format_desc[] = {
    "",
    "8 bit PseudoColor (dithering)",
    "8 bit StaticGray",
    "15 bit TrueColor (LE)",
    "16 bit TrueColor (LE)",
    "15 bit TrueColor (BE)",
    "16 bit TrueColor (BE)",
    "24 bit TrueColor (LE: bgr)",
    "32 bit TrueColor (LE: bgr-)",
    "24 bit TrueColor (BE: rgb)",
    "32 bit TrueColor (BE: -rgb)",
    "16 bit TrueColor (lut)",
    "32 bit TrueColor (lut)",
    "16 bit YUV 4:2:2",
    "12 bit YUV 4:2:0",
    "12 bit YUV 4:1:1"
};
//#define v4lDEBUG

static bool vershown = false;
static bool vbishown = false;

int avm_debug_level = 0;


//==============================================================================
#define __MODULE__ "v1lx"
v4lxif::v4lxif(const char * _device, const char* _vbidev, v4lxif_version _ifv )
: ifv(_ifv), device(_device)
{
    avml(AVML_DEBUG1,"v4lxif: start\n");

    // validate the name of device
    const char * p=_device;
    while(*p)
    {
	if (((*p>0) && (*p<32)) || (*p<0))
	{
	    avml(AVML_WARN, "Invalid video device filename %s suspected\n", _device);
	    avml(AVML_WARN, "Setting to default value ( /dev/video )\n");
	    device=_device="/dev/video";
	    break;
	}
	p++;
    }

    avml(AVML_DEBUG, "v4l1: Using interface %s in ::v4lxif",ifname[ifv].str);

    // open v4l device
    avml(AVML_DEBUG1, "v4lxif: open v4linux\n");
    devv4l=::open(device,O_RDWR);
    if (devv4l == -1)
	throw FATAL("Error opening v4lx device %s: %s in ::v4lxif",device,strerror(errno));

    int tmp;
    int driver = ioctl(devv4l, BTTV_VERSION, &tmp);
    if (driver >= 0)
    {
        if (!vershown)
	    avml(AVML_INFO, "BTTV driver version %d.%d.%d detected\n",
		   driver>>16, (driver & 0xff00) >>8, driver & 0xff);
	vershown = true;
    }
    else
	avml(AVML_WARN, "Error querying BTTV driver version: %d ( %s )\n",
	       driver, strerror(errno));

    if((driver<0) || ((driver>>16)>0) || (((driver&0xff00)>>8)>=8))
    /**
	Versions of bttv before 0.8.x behave strangely when program
	opens /dev/video and /dev/vbi simultaneously.
	We assume that non-bttv video4windows devices do it correctly
	( does it work at all with non-bttv hardware? )
    **/
    {
        devvbi = ::open(_vbidev, O_RDONLY);
	if (devvbi<0)
	    avml(AVML_WARN, "Warning: failed to open /dev/vbi device ( %s ); closed captioning won't be available\n",
		   strerror(errno));
    }
    else
    {
	devvbi = -1;
	if (!vbishown)
	    avml(AVML_INFO, "Broken VBI implementation; disabling closed captioning\n");
        vbishown = true;
    }

    avml(AVML_DEBUG1, "v4lxif: end constructor\n");
}

int v4lxif::readvbi(void* buf, unsigned int cnt)
{
    if (devvbi < 0)
	return -1;
    mutex.Lock();
    int r = ::read(devvbi, buf, cnt);
    mutex.Unlock();
    return r;
}

#undef __MODULE__
#define __MODULE__ "v4l1"
v4lxif::~v4lxif()
{
    avml(AVML_DEBUG, "v4lx: Close device in ::~v4lxif");
    if (::close(devv4l) == -1)
	avml(AVML_WARN, "v4l1: Error closing the v4lx device %s: %s in ::v4lxif",
	     device, strerror(errno));
}

//==============================================================================
v4l1baseif::v4l1baseif( int fd )
{
    devv4l=fd;
}

v4l1baseif::v4l1baseif(const char* mem, const char * _device, int _bpp, int _palette )
: v4lxif( _device, "/dev/vbi", v4lxif::v4l1 ), vchan(0), vaudio(0), grabbermem(0), _state(1)
{
    avml(AVML_DEBUG1, "v4l1baseif: start\n");

    setupok = true;
    // capture off
    if (ioctl(devv4l, VIDIOCCAPTURE, &zero) == -1) {
	if (errno != EINVAL && errno !=EBUSY)
	throw FATAL("VIDIOCCAPTURE in ::v4l1baseif");
    }
    avml(AVML_DEBUG, "v4l1baseif: before capabilities\n");

    // get capabilities
    if (ioctl(devv4l,VIDIOCGCAP,&vcap) == -1)
	throw FATAL("VIDIOC_G_CAP in ::v4l1baseif");

    // get input sources (channels)
    if (vcap.channels)
	vchan = (struct video_channel*)calloc(vcap.channels, sizeof(struct video_channel));

    // get audios
    if (vcap.audios)
	vaudio = (struct video_audio*)calloc(vcap.audios, sizeof(struct video_audio));

    //    return;

#ifdef v4lDEBUG
    debug("v4l1: Grabber name: %s",vcap.name);
    for (unsigned i = 0; device_cap[i].str != NULL; i++)
	if (vcap.type & (1 << i))
	    debug("\t %s: %s",device_cap[i].str,device_cap[i].description);
    debug("v4l1: range of tv size : %dx%d => %dx%d",
	  vcap.minwidth,vcap.minheight,
	  vcap.maxwidth,vcap.maxheight);
    debug("v4l1: input channels: %d",vcap.channels);
#endif

    // get information about each channel
    for (int i = 0; i < vcap.channels; i++) {
	vchan[i].channel = i;
	if ( -1 == ioctl(devv4l,VIDIOCGCHAN,&vchan[i]) )
	    VIF_FATAL("v4l1: VIDIOC_G_CHAN in ::v4l1baseif");
    }
    achan= 0;

#ifdef v4lDEBUG
    for (int i = 0; i < vcap.channels; i++) {
	debug("\t [%d] %s: %s [%d], %s%s%s%s",i+1,
	      vchan[i].name,
	      (vchan[i].flags & VIDEO_VC_TUNER)   ? "has tuner"  : "no tuner",
	      vchan[i].tuners,
	      (vchan[i].flags & VIDEO_VC_AUDIO)   ? "audio, "  : "",
	      "",//(vchan[i].flags & VIDEO_VC_NORM)    ? "norm, "  : "",
	      (vchan[i].type & VIDEO_TYPE_TV)     ? "tv"     : "",
	      (vchan[i].type & VIDEO_TYPE_CAMERA) ? "camera" : "");
    }
#endif

    avml(AVML_DEBUG1, "v4l1baseif: set input source\n");

    // set v4l to the first available input source, also set first norm
    vchan[0].norm= 0;
    if (ioctl(devv4l, VIDIOCSCHAN, &vchan[0]) == -1)
	avml(AVML_WARN, "v4l1: you need a newer bttv version (>= 0.5.14)\n");

    // get information for each audio
    for (int i = 0; i < vcap.audios; i++) {
	vaudio[i].audio = i;
	if (ioctl(devv4l,VIDIOCGAUDIO,&vaudio[i]) == -1)
	    avml(AVML_WARN, "v4l1: VIDIOC_G_AUDIO in ::v4l1baseif\n");
    }
    aaudio= 0;

    // mute them all
    for (int i = 0; i < vcap.audios; i++)
	if (vaudio[i].flags & VIDEO_AUDIO_MUTABLE) {
	    vaudio[i].flags |= VIDEO_AUDIO_MUTE;
	    if (ioctl(devv4l,VIDIOCSAUDIO,&vaudio[i]) == -1)
		avml(AVML_WARN, "v4l1: VIDIOC_S_AUDIO in ::v4l1baseif\n");
	}
    smute= true;

#ifdef v4lDEBUG
    debug("v4l1: input audios: %d",vcap.audios);
    for (int i = 0; i < vcap.audios; i++) {
	debug("\t [%d] %s: %s%s vol=%d, bass=%d, ter=%d",i+1,vaudio[i].name,
	      (vaudio[i].flags & VIDEO_AUDIO_MUTABLE)?"mutable":"not mutable",
	      (vaudio[i].flags & VIDEO_AUDIO_MUTE)?"[unmuted]":"[muted]",
	      (vaudio[i].flags & VIDEO_AUDIO_VOLUME)?vaudio[i].volume:-1,
	      (vaudio[i].flags & VIDEO_AUDIO_BASS)?vaudio[i].bass:-1,
	      (vaudio[i].flags & VIDEO_AUDIO_TREBLE)?vaudio[i].treble:-1);
    }
#endif

    avml(AVML_DEBUG1, "v4l1baseif: check tuner\n");

    // check tuner ( are there any tv cards with more than one ??? )
    for (int i = 0; i < vcap.channels; i++) {
	if ( vchan[i].tuners > 1 ) {
	    avml(AVML_WARN,
		 "v4l1: Found more than one [%d] tuner for source channel %s\n"
                 "      Only the first tuner of channel %s will be supported!\n"
		 "      This warning can be ignored...\n",
		 vchan[i].tuners, vchan[i].name, vchan[i].name);
	}
    }
    atuner= 0;
    memset(&vtuner,0,sizeof(struct video_tuner));
    //  vtuner.tuner=atuner;
    if ( vcap.type & VID_TYPE_TUNER) {
	if ( -1 == ioctl(devv4l, VIDIOCGTUNER, &vtuner) )
	    VIF_FATAL("v4l1: VIDIOC_G_TUNER in ::v4l1baseif");

#ifdef v4lDEBUG
	debug("v4l1: tuner: %s %lu-%lu",vtuner.name,vtuner.rangelow,vtuner.rangehigh);
	debug("v4l1: tuner sees stereo: %s",vtuner.flags & VIDEO_TUNER_STEREO_ON?"yes":"no");
	//if ( vtuner.flags & VIDEO_TUNER_NORM ) {
	debug("v4l1: tuner supports modes (active: %s): ",norms[vtuner.mode].str);
	for (int i = 0; norms[i].str != NULL; i++) {
	    if (vtuner.flags & (1<<i)) {
		debug("\t %s",norms[i].str);
	    }
	}
	//} else debug("v4l1: tuner supports no modes (fixed: %s)",norms[vtuner.mode].str);
#endif
    }
    avml(AVML_DEBUG1, "v4l1baseif: get window param\n");

    // get window parameters
    if (ioctl(devv4l, VIDIOCGWIN, &vwin) == -1)
	VIF_FATAL("v4l1: VIDIOC_G_WIN in ::v4l1baseif");

    if (ioctl(devv4l, VIDIOCGPICT, &vpic) == -1)
	VIF_FATAL("v4l1: VIDIOC_G_PICT in ::v4l1baseif");

    avml(AVML_DEBUG, "v4l1:  picture:  brightness=%d hue=%d colour=%d contrast=%d\n",
	 vpic.brightness, vpic.hue, vpic.colour, vpic.contrast);
    avml(AVML_DEBUG, "v4l1:  picture : whiteness=%d depth=%d palette=%d[%s]\n",
	 vpic.whiteness, vpic.depth, vpic.palette, PALETTE(vpic.palette));


    // get frame buffer parameters
    if (ioctl(devv4l, VIDIOCGFBUF, &vbuf) == -1)
	VIF_FATAL("v4l1: ioctl VIDIOC_G_FBUF in ::v4l1baseif");

    vpic.palette = VIDEO_RGB15_LE;

    if (ioctl(devv4l, VIDIOCSPICT, &vpic) == -1)
	VIF_FATAL("v4l1: VIDIOC_S_PICT in ::v4l1baseif");

#ifdef v4lDEBUG
    // reread palette
    if (ioctl(devv4l, VIDIOCGPICT, &vpic) == -1)
	VIF_FATAL("v4l1: VIDIOC_G_PICT in ::v4l1baseif");
    debug("v4l1: palette is %d",vpic.palette);
#endif

    // chunk2

    // get infos about snapshot buffer memory size
    if (ioctl(devv4l, VIDIOCGMBUF, &(vmbuf)) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_MBUF in ::v4l1baseif\n");

    msize = vmbuf.size;

    avml(AVML_DEBUG1, "v4l1baseif: memsize\n");

    // Alloc memory for snapshot
    grabbermem = (char *)mmap(0,msize,PROT_READ|PROT_WRITE,MAP_SHARED,devv4l,0);
    if (grabbermem == MAP_FAILED) {
        grabbermem = NULL;
	avml(AVML_WARN, "v4l1: unable to allocate memory for snap shots and video clips!\n");
    }

    avml(AVML_DEBUG, "v4l1: bpp %d, bpl %d, width %d, height %d\n",vbuf.depth,vbuf.bytesperline,vwin.width,vwin.height);
    avml(AVML_DEBUG, "v4l1: video buffer: size %d, frames %d\n",vmbuf.size,vmbuf.frames);
    avml(AVML_DEBUG, "v4l1: memory for snap shots: %p\n", grabbermem);

    for (int i = 0; i < vcap.audios; i++)
	if (vaudio[i].flags & VIDEO_AUDIO_MUTABLE) {
	    vaudio[i].flags &= ~VIDEO_AUDIO_MUTE;
	    if ( -1 == ioctl(devv4l,VIDIOCSAUDIO,&vaudio[i]) )
		VIF_FATAL("v4l1: VIDIOC_S_AUDIO in ::v4l1baseif");
	}

    gsync=ggrab=0;
    geven=true;

    if ( !setupok ) exit(1);

    avml(AVML_DEBUG1, "v4l1baseif: constructor exit\n");
}

v4l1baseif::~v4l1baseif()
{
    avml(AVML_DEBUG1,"v4l1: desctructor\n");

    // mute all
    if(vaudio)
    {
	for (int i = 0; i < vcap.audios; i++)
	    if (vaudio[i].flags & VIDEO_AUDIO_MUTABLE) {
		vaudio[i].flags |= VIDEO_AUDIO_MUTE;
		if ( -1 == ioctl(devv4l,VIDIOCSAUDIO,&vaudio[i]) )
		    avml(AVML_WARN, "v4l1: VIDIOC_S_AUDIO in ::v4l1baseif");
	    }
	smute= true;
    }
#if 1
    if (ggrab > gsync) {
	if (ioctl(devv4l,VIDIOCSYNC,0) == -1)
	    avml(AVML_WARN, "v4l1: VIDIOCSYNC in ::~v4l1baseif\n");
	else
	    gsync++;
    }
#endif

    // capture off
    if (ioctl(devv4l, VIDIOCCAPTURE, &zero) == -1)
	avml(AVML_WARN, "v4l1: ioctl VIDIOCCAPTURE in v4l1baseif::~v4l1baseif\n");

    if (grabbermem)
	    munmap(grabbermem,msize);

    if (vchan)
	free(vchan);
    if (vaudio)
        free(vaudio);
}

void v4l1baseif::setCapture( bool on )
{
    avml(AVML_DEBUG1, "v4l1baseif: setCapture()\n");

    mutex.Lock();
    if ( -1 == ioctl(devv4l, VIDIOCCAPTURE, (on?&one:&zero) ) )
	VIF_FATAL("v4l1: VIDIOCCAPTURE in ::setCapture");
    scapture= on;
    mutex.Unlock();
}

void v4l1baseif::setFreq( unsigned long freq )
{
    if ( -1 == ioctl(devv4l, VIDIOCSFREQ, &freq) )
	VIF_FATAL("v4l1: VIDIOC_S_FREQ in ::setFreq");
}

void v4l1baseif::setPicBrightness( int bri )
{
#ifdef PREREAD
    if ( -1 == ioctl(devv4l, VIDIOCGPICT, &vpic) )
	VIF_FATAL("v4l1: VIDIOC_G_PICT in ::setBrightness");
#endif
    vpic.brightness= (bri+128)<<8;
    if ( -1 == ioctl(devv4l, VIDIOCSPICT, &vpic) )
	VIF_FATAL("v4l1: VIDIOC_S_PICT in ::setBrightness");
}

void v4l1baseif::setPicConstrast( int contr )
{
#ifdef PREREAD
    if ( -1 == ioctl(devv4l, VIDIOCGPICT, &vpic) )
	VIF_FATAL("v4l1: VIDIOC_G_PICT in ::setContrast");
#endif
    vpic.contrast= contr<<7;
    if ( -1 == ioctl(devv4l, VIDIOCSPICT, &vpic) )
	VIF_FATAL("v4l1: VIDIOC_S_PICT in ::setContrast");
}

void v4l1baseif::setPicColor( int color )
{
#ifdef PREREAD
    if ( -1 == ioctl(devv4l, VIDIOCGPICT, &vpic) )
	VIF_FATAL("v4l1: VIDIOC_G_PICT in ::setColor");
#endif
    vpic.colour= color<<7;
    if ( -1 == ioctl(devv4l, VIDIOCSPICT, &vpic) )
	VIF_FATAL("v4l1: VIDIOC_S_PICT in ::setColor");
}

void v4l1baseif::setPicHue( int hue )
{
#ifdef PREREAD
    if(ioctl(devv4l, VIDIOCGPICT, &vpic) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_PICT in ::setHue\n");
#endif
    vpic.hue= (hue+128)<<8;
    if (ioctl(devv4l, VIDIOCSPICT, &vpic) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_S_PICT in ::setHue\n");
}

void v4l1baseif::setChannel( int source )
{
    if((source<0)||(source>=vcap.channels))
    {
	avml(AVML_WARN, "warning: no such channel %d\n", source);
	return;
    }

    avml(AVML_DEBUG, "v4l1: setChannel %d\n",source);
    if (ioctl(devv4l, VIDIOCSCHAN, &vchan[source]) == -1)
    {
	VIF_WARN("v4l1: VIDIOC_S_CHAN in ::setChannel");
	return;
    }
    achan= source;
    if(source<vcap.audios)
	aaudio= source;
}

void v4l1baseif::setChannelName( const char* name )
{
    int source = 0;

    while (source < vcap.channels)
    {
	//printf("name %s\n", capChannelName(source));
	if (strcasecmp(capChannelName(source), name) == 0)
	    break;
        source++;
    }
    if (source >= vcap.channels)
	return;

    avml(AVML_DEBUG, "v4l1: setChannel %d\n", source);

    if (ioctl(devv4l, VIDIOCSCHAN, &vchan[source]) == -1)
    {
	avml(AVML_WARN, "v4l1: VIDIOC_S_CHAN in ::setChannel\n");
	return;
    }
    achan = source;
    if(source<vcap.audios)
	aaudio = source;
}

void v4l1baseif::setChannelNorm( int norm )
{
    avml(AVML_DEBUG, "v4l1: setChannelNorm %d\n", norm);
    if (ioctl(devv4l, VIDIOCSCHAN, &vchan[achan]) == -1)
    {
	avml(AVML_WARN, "v4l1: VIDIOC_S_CHAN in ::setChannelNorm\n");
	return;
    }
    vchan[achan].norm= norm;
}

void v4l1baseif::setTuner( int no )
{
    atuner= no;
}

void v4l1baseif::setTunerMode( int norm )
{
    vtuner.tuner= atuner;
#ifdef PREREAD
    if (ioctl(devv4l, VIDIOCGTUNER, &vtuner) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_TUNER in ::setTunerMode\n");
#endif
    vtuner.mode= norm;
    if (ioctl(devv4l, VIDIOCSCHAN, &vtuner) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_S_TUNER in ::setTunerMode\n");
    avml(AVML_WARN, "v4l1: Use ::setChannelNorm to set input NORM in ::setTunerMode\n");
}

void v4l1baseif::setAudioMute( bool on )
{
    if (aaudio >= vcap.audios)
    {
	avml(AVML_WARN, "WARNING: no such audio channel %d\n", aaudio);
	return;
    }
    if ( vaudio[aaudio].flags & VIDEO_AUDIO_MUTABLE ) {
	if (ioctl(devv4l,VIDIOCGAUDIO,&vaudio[aaudio]) == -1)
	    avml(AVML_WARN, "v4l1: VIDIOC_G_AUDIO in ::setMute\n");

	if ( on )
	    vaudio[aaudio].flags |= VIDEO_AUDIO_MUTE;
	else
	    vaudio[aaudio].flags &= ~VIDEO_AUDIO_MUTE;

	if (ioctl(devv4l,VIDIOCSAUDIO,&vaudio[aaudio]) == -1)
	    avml(AVML_WARN, "v4l1: VIDIOC_S_AUDIO in ::setMute\n");

	smute= on;
    }

    avml(AVML_DEBUG, "v4l1: setAutioMute [%d] %s\n",
	 aaudio,smute ? "muted" : "unmuted");
}

void v4l1baseif::setAudioVolume( int vol )
{
    if (aaudio>=vcap.audios)
    {
	avml(AVML_WARN, "WARNING: no such audio channel %d\n", aaudio);
	return;
    }
#ifdef PREREAD
    if (ioctl(devv4l, VIDIOCGAUDIO, &vaudio[aaudio]) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_AUDIO in ::setVolume\n");
#endif
    vaudio[aaudio].volume = vol;
    if (ioctl(devv4l, VIDIOCSAUDIO, &vaudio[aaudio]) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_S_AUDIO in ::setVolume\n");
}

void v4l1baseif::setAudioMode( int mode )
{
    if (aaudio >= vcap.audios)
    {
	avml(AVML_WARN, "WARNING: no such audio channel %d\n", aaudio);
	return;
    }
#ifdef PREREAD
    if (ioctl(devv4l,VIDIOCGAUDIO,&vaudio[aaudio]) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_AUDIO in ::setAudioMode\n");
#endif

    avml(AVML_DEBUG, "v4l1: setAudioMode= %d [%s]\n", mode, audiodescr[mode]);

    vaudio[aaudio].mode = mode;
    if (ioctl(devv4l,VIDIOCSAUDIO,&vaudio[aaudio]) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_S_AUDIO in ::setAudioMode\n");
}

void v4l1baseif::setPalette( int pal )
{
    vpic.palette = VIDEO_RGB15_LE;

    if (ioctl(devv4l, VIDIOCSPICT, &vpic) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_S_PICT in ::v4l1baseif");
}

bool v4l1baseif::getAudioMute()
{
    if (ioctl(devv4l,VIDIOCGAUDIO,&vaudio[aaudio]) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_AUDIO in ::getMute");

    avml(AVML_DEBUG, "v4l1: getAudioMute %s, %d",
	 (vaudio[aaudio].flags & VIDEO_AUDIO_MUTE)?"true":"false",
	 vaudio[aaudio].flags);

    return ( vaudio[aaudio].flags & VIDEO_AUDIO_MUTE );
}

int  v4l1baseif::getAudioVolume()
{
    if (ioctl(devv4l,VIDIOCGAUDIO,&vaudio[aaudio]) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_AUDIO in ::getVolume\n");

    return vaudio[aaudio].volume;
}

int  v4l1baseif::getAudioMode()
{
    if (ioctl(devv4l,VIDIOCGAUDIO,&vaudio[aaudio]) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_AUDIO in ::getAudioMode\n");

    avml(AVML_DEBUG, "v4l1: getAudioMode= %d\n",vaudio[aaudio].mode);

    return vaudio[aaudio].mode;
}

unsigned long v4l1baseif::getTunerSignal()
{
    vtuner.tuner= atuner;
    if (ioctl(devv4l, VIDIOCGTUNER, &vtuner) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_TUNER in ::getTunerSignal\n");

    return vtuner.signal;
}
int v4l1baseif::getTunerMode()
{
    vtuner.tuner = atuner;
    if (ioctl(devv4l, VIDIOCGTUNER, &vtuner) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_TUNER in ::getTunerMode\n");

    return vtuner.mode;
}
int v4l1baseif::getTunerFlags()
{
    vtuner.tuner = atuner;
    if (ioctl(devv4l, VIDIOCGTUNER, &vtuner) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_TUNER in ::getTunerFlags\n");

    return vtuner.flags;
}

bool v4l1baseif::capAudioVolume()
{
    if (ioctl(devv4l,VIDIOCGAUDIO,&vaudio[aaudio]) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_AUDIO in ::capAudioVolume\n");

    return ( vaudio[aaudio].flags & VIDEO_AUDIO_VOLUME );
}
bool v4l1baseif::capAudioMutable()
{
    if (ioctl(devv4l,VIDIOCGAUDIO,&vaudio[aaudio]) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_AUDIO in ::capAudioMutable\n");

    return vaudio[aaudio].flags & VIDEO_AUDIO_MUTABLE;
}

bool v4l1baseif::capTunerStereo()
{
    vtuner.tuner= atuner;
    if (ioctl(devv4l, VIDIOCGTUNER, &vtuner) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_TUNER in ::getTunerMode\n");

    return vtuner.flags & VIDEO_TUNER_STEREO_ON;
}

bool v4l1baseif::capTunerNorm()
{
    vtuner.tuner = atuner;
    if (ioctl(devv4l, VIDIOCGTUNER, &vtuner) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_TUNER in ::getTunerMode\n");

    return vtuner.flags & VIDEO_TUNER_NORM;
}

void v4l1baseif::capCapSize( unsigned int *minw, unsigned int *minh, unsigned int *maxw, unsigned int *maxh )
{
#if 0
    *minw= vcap.minwidth; *minh= vcap.minheight;
    *maxw= vcap.maxwidth; *maxh= vcap.maxheight;
#else
    *minw= vcap.minwidth;
    *minh= vcap.minheight;
    *maxw= maxwidth[vchan[achan].norm];
    *maxh= maxheight[vchan[achan].norm];
#endif
#ifdef v4lDEBUG
    debug("v4l1: minw %d, maxw %d, minh %d, maxh %d in ::capCapSize",*minw,*maxw,*minh,*maxh);
#endif
}

int v4l1baseif::addCapAClip( int x1, int y1, unsigned int x2,
			    unsigned int y2, int xadj, int yadj )
{
    //    return 0;
    if ( vcap.type & VID_TYPE_CLIPPING ) {
#ifdef v4lDEBUG
	debug("v4l1: addCapAClip: [%d] %dx%d+%d+%d",nrofclips,x2-x1,y2-y1,x1-xadj,y1-yadj);
#endif
	if (cliprecs[nrofclips].x !=  x1 - xadj
	    || cliprecs[nrofclips].y != y1 - yadj
	    || cliprecs[nrofclips].width != (int)x2 - x1
	    || cliprecs[nrofclips].height != (int)y2 - y1 )
	    clipTabChanged= true;

	cliprecs[nrofclips].x = x1 - xadj;
	cliprecs[nrofclips].y = y1 - yadj;
	cliprecs[nrofclips].width = x2 - x1;
	cliprecs[nrofclips].height = y2 - y1;
	nrofclips++;
    }
    return nrofclips;
}

int v4l1baseif::setCapAClip( int x, int y, unsigned int width, unsigned int height )
{
    //    return 0;
    x &= ~1;
    y &= ~1;
    width &= ~1;
    height &= ~1;
#if 0
    if ( -1 == ioctl(devv4l, VIDIOCGWIN, &vwin) )
	VIF_FATAL("v4l1: VIDIOC_G_WIN in ::setCapAClip");
#endif
    avml(AVML_DEBUG, "v4l1: setCapAClip: flags %d, chromakey %d",vwin.flags,vwin.chromakey);

    mutex.Lock();
    if (vcap.type & VID_TYPE_CHROMAKEY)
	vwin.chromakey = 0;    /* XXX */

    vwin.flags = 0;
    if (vcap.type & VID_TYPE_CLIPPING) {
	vwin.clips = cliprecs;
	vwin.clipcount = nrofclips;
    }

    vwin.x = x;
    vwin.y = y;
    vwin.height = height;
    vwin.width = width;

    mutex.Unlock();
    avml(AVML_DEBUG, "v4l1: setCapAClip new values: %d, %d, %d, %d",x,y,width,height);
    return nrofclips;
}

int v4l1baseif::applyCapAClip( int n )
{
    // save old values
    int ox = vwin.x;
    int oy = vwin.y;
    int ow = vwin.width;
    int oh = vwin.height;

    avml(AVML_DEBUG, "v4l1: applyCapAClip given %d, %d, %d, %d [%d]\n",
	 ox, oy, ow, oh, nrofclips);

    vwin.clipcount = n;
    mutex.Lock();
    if (ioctl(devv4l, VIDIOCSWIN, &vwin) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_S_WIN in ::setCapAClip");

#if 1
#if 0
    if ( -1 == ioctl(devv4l, VIDIOCGWIN, &vwin) )
	VIF_FATAL("v4l1: VIDIOC_G_WIN in ::setCapAClip");
    if ( vwin.x != x ) vwin.x = x;
    if ( vwin.y != y ) vwin.y = y;
    if ( vwin.height != height ) vwin.height = height;
    if ( vwin.width != width ) vwin.width = width;
#else
    // set old values
    vwin.x = ox;
    vwin.y = oy;
    vwin.height = oh;
    vwin.width = ow;
#endif
#endif

#if 0
    vpic.palette = x11_format;

#ifdef v4lDEBUG
    //debug("v4l1: palette=%s",PALETTE(vpic.palette));
#endif

    if (ioctl(devv4l, VIDIOCSPICT, &vpic) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_S_PICT in ::v4l1baseif\n");

    if (ioctl(devv4l, VIDIOCGPICT, &vpic) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_G_PICT in ::v4l1baseif\n");

#endif
    mutex.Unlock();
    return n;
}

void v4l1baseif::getCapAClip(int *x, int *y, unsigned int *width, unsigned int *height)
{
#if 0
    if (ioctl(devv4l, VIDIOCGWIN, &vwin) == -1)
	VIF_FATAL("v4l1: VIDIOC_G_WIN in ::getCapAClip");
#endif
    *x = vwin.x;
    *y = vwin.y;
    *width = vwin.width;
    *height = vwin.height;

    avml(AVML_DEBUG, "v4l1: getCapAClip: %d, %d, %d, %d\n",*x,*y,*width,*height);
}

bool v4l1baseif::grabSetParams( int width, int height, int palette )
{
    avml(AVML_INFO, "v4l1baseif::grabSetParams(%d, %d, %d)\n", width, height, palette);

    vgrab[0].width= width;
    vgrab[0].height= height;
    vgrab[0].frame=0;
    vgrab[0].format=palette;

    memcpy(vgrab + 1, vgrab, sizeof(struct video_mmap));
    vgrab[1].frame=1;

    grabCapture( true );

    return true;
}
char * v4l1baseif::grabCapture( bool single )
{
    // even frame: 0
    // odd frame:  1
    //printf("v4lxif::grabCapture(%s)\n", single ? "true" : "false");
    //printf("v4lxif::grabCapture %d  %d   %d\n", ggrab, gsync, geven);
    char * rmem;
    if ( !single && (ggrab == gsync) )
	if ( !grabOne( geven ? 0 : 1 ) ) return NULL;

    if ( !grabOne( geven ? 1 : 0 ) ) return NULL;
    if ( ggrab > gsync+1 ) {
	grabWait( geven ? 0 : 1 );
	rmem = (grabbermem + vmbuf.offsets[ geven ? 0 : 1 ]);
    } else {
	grabWait( geven ? 1 : 0 );
	rmem = (grabbermem + vmbuf.offsets[ geven ? 1 : 0 ]);
    }
    geven = !geven;

    return rmem;
}

bool v4l1baseif::grabOne( int frame )
{
    if (ioctl(devv4l,VIDIOCMCAPTURE,&(vgrab[frame])) == -1) {
	if (errno == EAGAIN) {
	    avml(AVML_WARN, "v4l1: Grabber chip can't sync\n");
	    return false;
	}
	avml(AVML_WARN, "v4l1: VIDIOCMCAPTURE in ::grabOne: %s\n", strerror(errno));
    }
    ggrab++;
    return true;
}

void v4l1baseif::grabWait( int frame )
{
    if (ioctl(devv4l,VIDIOCSYNC,&(vgrab[frame].frame)) == -1)
	avml(AVML_WARN, "v4l1: VIDIOCSYNC in ::grabWait (frame %d)\n", frame);
    else gsync++;
}

v4l1if::v4l1if(const char* mem, const char * _device, int _bpp, int _palette)
  : v4l1baseif(mem, _device, _bpp, _palette)
{
    avml(AVML_DEBUG1, "v4l1if: constructor\n");

    int visual_class;

#ifndef X_DISPLAY_MISSING
    const char* xdname = XDisplayName(0);
    Display* disp= XOpenDisplay(xdname);
    Visual *myvisual;
#ifdef v4lDEBUG
    List_visuals(disp);
#endif
    // get the visual
    int display_bits= find_visual(disp,NULL,&myvisual,&visual_class);

    // do some checks for color depth, size and buffer pos
    // first, guess the color depth of the screen

    int n, pixmap_bytes = 0;
    XPixmapFormatValues *pf = XListPixmapFormats(disp, &n);
    int planes = DefaultDepth(disp, DefaultScreen(disp));
    for (int i = 0; i < n; i++) {
	if (pf[i].depth == planes)
	    pixmap_bytes = pf[i].bits_per_pixel / 8;
    }

    /* guess physical screen format */
    bool be = (ImageByteOrder(disp) == MSBFirst);

    switch (pixmap_bytes) {
    case 1: x11_format = VIDEO_RGB08; break;
    case 2: x11_format = (display_bits==15) ?
	((be) ?  VIDEO_RGB15_BE : VIDEO_RGB15_LE)
	    : ((be) ? VIDEO_RGB16_BE : VIDEO_RGB16_LE) ; break;
    case 3: x11_format = (be) ? VIDEO_RGB24 : VIDEO_BGR24; break;
    case 4: x11_format = (be) ? VIDEO_RGB32 : VIDEO_BGR32; break;
    default:
	avml(AVML_WARN, "v4l1: Unknown color depth found: %d\n", pixmap_bytes);
    }

#ifdef v4lDEBUG
    debug("v4l1: hope physical screen format is <%s>",format_desc[x11_format]);
#endif
#endif /* X_DISPLAY_MISSING */

    if ( _palette > 0 && _palette < 11 ) {
	x11_format = _palette;
    }
    vpic.palette = (x11_format < sizeof(format2palette)/sizeof(unsigned short)) ?
	format2palette[x11_format] : 0;
    if ( vpic.palette == 0 ) {
	avml(AVML_WARN, "v4l1: unsupported overlay video format <%d-%s>\n",
	     x11_format,format_desc[x11_format]);
	vpic.palette = VIDEO_RGB08;
    }

    if (ioctl(devv4l, VIDIOCSPICT, &vpic) == -1)
	VIF_FATAL("v4l1: VIDIOC_S_PICT in ::v4l1if");

#ifdef v4lDEBUG
    // reread palette
    if (ioctl(devv4l, VIDIOCGPICT, &vpic) == -1)
	VIF_FATAL("v4l1: VIDIOC_G_PICT in ::v4l1if");
    debug("v4l1: palette is %d",vpic.palette);
#endif

    avml(AVML_DEBUG1, "v4l1if: test dga\n");

#ifdef HAVE_LIBXXF86DGA // do some strange things with dga
    int major, minor, width, bank, ram;
    int flags;
    void *base = NULL;
    bool have_dga= false;

#ifndef XDGA_MAJOR_VERSION
#define XDGA_MAJOR_VERSION XF86DGA_MAJOR_VERSION
#endif

#ifndef XDGA_MINOR_VERSION
#define XDGA_MINOR_VERSION XF86DGA_MINOR_VERSION
#endif

    if (XF86DGAQueryExtension(disp, &major, &minor)) {
	XF86DGAQueryDirectVideo(disp,XDefaultScreen(disp),&flags);
	if (flags & XF86DGADirectPresent) {
	    have_dga= true;
	    XF86DGAQueryVersion(disp,&major,&minor);
	    if ((major != XDGA_MAJOR_VERSION) || (minor != XDGA_MINOR_VERSION)) {
		avml(AVML_DEBUG1,
		     "v4l1: X-Server DGA extension version mismatch, disabled\n"
		     "v4l1: server version %d.%d != included version %d.%d\n",
		     major,minor, XDGA_MAJOR_VERSION,XDGA_MINOR_VERSION);
		have_dga= false;
	    } else {
		XF86DGAGetVideoLL(disp, DefaultScreen(disp), (int*)&base, &width, &bank, &ram);
		if (!base)
		    avml(AVML_WARN,
			 "v4l1: can not allocate frame buffer base: %p\n", base);
	    }
	}
    }

    if (have_dga) {
	avml(AVML_DEBUG1, "v4l1if: have dga\n");
	// check framebuffer
	if (((unsigned long)vbuf.base & 0xfffff000) !=
	    ((unsigned long)base & 0xfffff000) ) {
	    avml(AVML_DEBUG1,
		 "v4l1: Video4Linux and DGA disagree about the framebuffer base\n"
		 "v4l1: Video4Linux: %p, dga: %p!\n"
		 "v4l1: you probably want to insmod the bttv module with "
		 "\"vidmem=0x%03lx\"\n",
		 vbuf.base, base, (unsigned long)base >> 20);
	    setupok=false;
	}
	if ( _bpp == 0  && _palette == 0 ) {
	    // check color depth
	    if ( (unsigned int)((vbuf.depth+7)&0xf8) != format2depth[x11_format] ) {
		avml(AVML_DEBUG1,
		     "v4l1: Video4Linux and DGA disagree about the color depth\n"
		     "v4l1: Video4Linux: %d, DGA: %d.\n"
		     "v4l1: Is kv4lsetup installed correctly?\n",
		     (vbuf.depth+7)&0xf8,format2depth[x11_format]);
		setupok=false;
	    }
	}
#ifdef v4lDEBUG
	else debug("v4l1: forced bpp %d, palette %d",_bpp,_palette);
#endif
    } // have_dga
#endif /* HAVE_LIBXXF86DGA */
}

void v4l1if::setPalette( int pal )
{
    x11_format = ((unsigned int)pal < sizeof(format2palette)/sizeof(unsigned short))?format2palette[pal]:0;
    vpic.palette= x11_format;

    if (vpic.palette == 0)
	avml(AVML_WARN, "v4l1: unsupported overlay video format <%s> in ::v4l1baseif\n",
	     format_desc[x11_format]);

    if (ioctl(devv4l, VIDIOCSPICT, &vpic) == -1)
	avml(AVML_WARN, "v4l1: VIDIOC_S_PICT in ::v4l1baseif");
}
