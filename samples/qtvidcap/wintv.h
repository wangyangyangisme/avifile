/*
    xWinTV using Bt848 frame grabber driver

    Copyright (C) 1998 Moritz Wenk (wenk@mathematik.uni-kl.de)

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

#ifndef WINTV_H
#define WINTV_H

// for debugging
//
// myDEBUG     to debug all
// sDEBUG      mixer
// tvDEBUG     winTVScreen
// rcDEBUG     rcfile
// vtxDEBUG    vtx
// X11DEBUG    x11-events
//#define X11DEBUG

// debug everything
// #define myDEBUG // this can be defined with ./configure --enable-debug

// need this to avoid conflicting type declarations in Xmd.h and qglobal.h
#define QT_CLEAN_NAMESPACE

// this is obsolete and should be removed in later versions!
#define withTVscreen

// change this values to adjust range and step of mixer slider
#define MIXER_MAX_VALUE 100
#define MIXER_STEP 3

// range is -FINETUNERANGE*4 ... FINETUNERANGE*4
#define FINETUNERANGE 16

// signal quality must be better than this value to detect a vaild channel
#define SIGQUALITY 0

#define TIMEWAIT 500


#define OP_CAP        1
#define OP_BPP        2
#define OP_VME        3
#define OP_DEV        4
#define OP_SHI        5
#define OP_REF        6
#define OP_PAL        7
#define OP_DOK        8

/* ------------------- some definitions ---------------------------------*/

// aspect ratio
#define ASPECT_NONE  0
#define ASPECT_16_9  1
#define ASPECT_4_3   2

#define ASPECT_FIX_BOTH  0
#define ASPECT_FIX_HOR   1
#define ASPECT_FIX_VER   2

// some defaults
#define DCOL 254
#define DBRI 0
#define DHUE 0
#define DCONTRAST 216
#define DNORM  0
#define DIN 0
#define DFREQ 772
#define DCHANNEL 1

#define PICNAME "image"
#define VIDNAME "video"

#define NONE 0

#define DEFAULT_WIDTH   320
#define DEFAULT_HEIGHT   240

#define PAL_WIDTH 768
#define PAL_WIDE_WIDTH 922
#define PAL_HEIGHT 576

#define NTSC_WIDTH 640
#define NTSC_HEIGHT 480

// several delay times
#define DEFAULT_DELAY 200       // 0.2 sec
#define AUDIOMODE_DELAY 1000     // 1 sec.
#define INFO_DELAY 3000         // 3 sec.

// audio
#define AUDIOMODE_AUTODETECT 256
#define CAN_AUDIO_VOLUME     1

// supported video clip formats
#define VIDEOCLIP_AVI 0
#define VIDEOCLIP_PPM 1
#define VIDEOCLIP_RAW 2
#define VIDEOCLIP_IV5 3

// supported shap shot formats
#define SNAPSHOT_PNM  0
#define SNAPSHOT_JPEG 1
#define SNAPSHOT_TIFF 2
#define SNAPSHOT_PNG  3
#define SNAPSHOT_GIF  4

#define CHINFO_NODISPVAL -1024
#define OSD_LEVEL_OFF   0
#define OSD_LEVEL_1   1
#define OSD_LEVEL_2   2

#define VIEWREFRESH_ALLOW      1
#define VIEWREFRESH_VISIBILITY 2

#endif // WINTV_H
