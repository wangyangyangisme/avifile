/*
 kwintv, Video4Linux compatible KDE application

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

#ifndef V4LIF_H
#define V4LIF_H

/* GUILESS should be defined if this interface will be used for a text-console
 application - define it in config.h (using autoconf) */

#include <avm_default.h>
#include <avm_locker.h>
#include "wintv.h"

#include <sys/types.h>

/* Necessary to prevent collisions between <linux/time.h> and <sys/time.h> when V4L2 is installed. */
#define _LINUX_TIME_H
#include <linux/videodev.h>

#define V4L_DEVICE "/dev/video"
#define V4L_VBI_DEVICE "/dev/vbi"

/* WANT_V4L_DEBUG should be set in config.h/autoconf */
#define MAXCLIPRECS 100

#define PALETTE(x) ((x < sizeof(device_pal)/sizeof(char*)) ? device_pal[x] : "UNKNOWN")

struct STRTAB {
    int nr;
    const char* str;
};

//--------------------------------------------------------------------------------
//--------------------- the virtual interface ------------------------------------
//--------------------------------------------------------------------------------

class v4lxif {
public:
    enum v4lxif_version { none, v4l1, v4l2 };

    v4lxif( const char * _device = V4L_DEVICE, const char * _vbi_dev=V4L_VBI_DEVICE, v4lxif_version _ifv = v4l1 );
    virtual ~v4lxif();

    v4lxif_version interface() { return ifv; }

    bool hasvbi() { return (devvbi>=0); }
    int getvbidev() { return devvbi; }
    int readvbi(void* buf, unsigned int cnt);
    // rest is virtual

    // set
    virtual void setCapture( bool on ) = 0;          // de/activate overlay capturing
    virtual void setFreq( unsigned long freq ) = 0;  // in 1/16th MHz

    virtual void setPicBrightness( int bri ) = 0;    //  -127 .. 127
    virtual void setPicConstrast( int contr ) = 0;   //  0 .. 511
    virtual void setPicColor( int color ) = 0;       //  0 .. 511
    virtual void setPicHue( int hue ) = 0;           //  -127 .. 127

    virtual void setChannel( int source ) = 0;       // input source like Tuner, Camera
    virtual void setChannelName( const char* name ) = 0; //  input channel, Tuner,
    virtual void setChannelNorm( int norm ) = 0;     // VIDEO_TUNER_PAL, _NTSC, _SECAM, _AUTO

    virtual void setTuner( int no ) = 0;             // Nr of Tuner (any card aviable with more than one ??)
    virtual void setTunerMode( int norm ) = 0;       // (VIDEO_TUNER_PAL, _NTSC, _SECAM, _AUTO)

    virtual void setAudioMute( bool on ) = 0;        // toggle mute
    virtual void setAudioVolume( int vol ) = 0;      // set volume of TV card
    virtual void setAudioBalance( int bal ) = 0;     // set balance
    virtual void setAudioMode( int mode ) = 0;       // VIDEO_SOUND_MONO, _STEREO, _LANG1, _LANG2

    virtual void setPalette( int pal ) = 0;          // set the pallette

    // get
    virtual bool getCapture() = 0;                   // overlay captureing enabled?

    virtual int getChannelActive() = 0;              // get active input source

    virtual int getTunerActive() = 0;                // get active tuner
    virtual int getTunerMode() = 0;                  // VIDEO_MODE_PAL, _NTSC, _SECAM, _AUTO
    virtual unsigned long getTunerSignal() = 0;      // Signal strength if known - between 0-65535
    virtual int getTunerFlags() = 0;                 // VIDEO_TUNER_*

    virtual int getAudioActive() = 0;                // get active audio
    virtual bool getAudioMute() = 0;                 // get mute state
    virtual int  getAudioVolume() = 0;               // get vol
    virtual int  getAudioBalance() = 0;              //
    virtual int  getAudioMode() = 0;                 // VIDEO_SOUND_MONO, _STEREO, _LANG1, _LANG2

    // getChannel
    virtual int getChannelType() = 0;                // VIDEO_TYPE_TV, _CAMERA
    virtual int getChannelNorm() = 0;                // norm set for this channel

    // capAutio
    virtual bool capAudioVolume() = 0;               // tv card capable to set volume?
    virtual bool capAudioMutable() = 0;              // tv card capable to mute/unmute?

    // capTuner
    virtual bool capTunerStereo() = 0;               // tuner sees stereo?
    virtual bool capTunerNorm() = 0;                 // tuner has settable norm

    // capChannel
    virtual bool capChannelAudio( int no ) = 0;      // active channel has audio
    virtual bool capChannelNorm( int no ) = 0;       // " has setable norm
    virtual bool capChannelTuner(int no  ) = 0;      // " has a tv tuner
    virtual char * capChannelName( int no ) = 0;     // name of channel

    // capCap
    virtual int capCapChannelC() = 0;                // number of channels ( = input devices )
    virtual int capCapAudioC() = 0;                  // " of audios
    virtual char * capCapName() = 0;                 // name of interface
    virtual int capCapType() = 0;                    // type ", VID_TYPE_*
    virtual void capCapSize( unsigned int *minw,              /* TV screen min/max size */
			     unsigned int *minh,
			     unsigned int *maxw,
			     unsigned int *maxh ) = 0;

    // clipping stuff for capture area setup
    virtual void resetCapAClip() = 0;
    virtual int addCapAClip( int x1, int y1, unsigned int x2, unsigned int y2, int xadj=0, int yadj=0 ) = 0;
    virtual int setCapAClip( int x,int y, unsigned int width, unsigned int height ) = 0;
    virtual void getCapAClip( int *x,int *y, unsigned int *width, unsigned int *height ) = 0;
    virtual int applyCapAClip( int nclips ) = 0;
    virtual bool changedCapAClip() = 0;

    // additional
    virtual int getWWidth() = 0;
    virtual int getWHeight() = 0;

    // frame buffer
    virtual unsigned int getFBheight() = 0;
    virtual unsigned int getFBwidth() = 0;
    virtual unsigned int getFBdepth() = 0;
    virtual unsigned int getFBbpl() = 0;

    // capture image stuff
    virtual bool grabSetParams( int width, int height, int palette ) = 0;
    virtual char * grabCapture( bool single ) = 0;   // single=false for stream captureing

    virtual int isOk() =0;
protected:
    v4lxif_version ifv;
    avm::PthreadMutex mutex;
    const char * device;      // name of the video4linux-device
    int devvbi;
    int devv4l;               // the video4linux-device
};

//--------------------------------------------------------------------------------
//--------------------- the video 4 linux interface -------------------------------
//--------------------------------------------------------------------------------

class v4l1baseif : public v4lxif {
public:
    v4l1baseif( const char* mem=0, const char * _device = V4L_DEVICE, int _bpp = 0, int _palette = 0 );
    v4l1baseif( int );
    ~v4l1baseif();


    void setCapture( bool on );
    void setFreq( unsigned long freq );

    void setPicBrightness( int bri );         //  -127 .. 127
    void setPicConstrast( int contr );        //  0 .. 511
    void setPicColor( int color );            //  0 .. 511
    void setPicHue( int hue );                //  -127 .. 127

    void setChannel( int source );            //  input channel, Tuner,
    void setChannelName( const char* name );  //  input channel, Tuner,
    void setChannelNorm( int norm );

    void setTuner( int no );                  //  tuner, if more than one exists
    void setTunerMode( int norm );            //  PAL, NTSC, SECAM, AUTO

    void setAudio( int source );
    void setAudioMute( bool on );
    void setAudioVolume( int vol );
    void setAudioBalance( int bal ) {}
    void setAudioMode( int mode );

    virtual void setPalette( int pal );

    bool getCapture() { return scapture; }

    // getTuner
    int getTunerActive() { return atuner; }
    int getTunerMode();
    unsigned long getTunerSignal();
    int getTunerFlags();

    // getAudio
    int getAudioActive() { return aaudio; }
    bool getAudioMute();
    int  getAudioVolume();
    int  getAudioBalance() { return 0; }
    int  getAudioMode();

    // getChannel
    int getChannelType() { return vchan[achan].type; }
    int getChannelNorm() { return vchan[achan].norm; }
    int getChannelActive() { return achan; }

    // capAudio
    bool capAudioVolume();
    bool capAudioMutable();

    // capTuner
    bool capTunerStereo();
    bool capTunerNorm();

    // capChannel
    bool capChannelAudio( int no ) { return (vchan[no].flags & VIDEO_VC_AUDIO); }
    bool capChannelNorm( int no ) { return true; /*return vchan[no].flags & VIDEO_VC_NORM;*/ }
    bool capChannelTuner( int no ) { return (vchan[no].flags & VIDEO_VC_TUNER); }
    char * capChannelName( int no ) { return vchan[no].name; }

    // capCap
    int capCapChannelC() { return vcap.channels; }
    int capCapAudioC() { return vcap.audios; }
    char * capCapName() { return &(vcap.name[0]); }
    int capCapType() { return vcap.type; }
    void capCapSize( unsigned int *minw, unsigned int *minh, unsigned int *maxw, unsigned int *maxh );

    // clipping stuff
    void resetCapAClip() { nrofclips= 0; clipTabChanged= false; }
    int addCapAClip( int x1, int y1, unsigned int x2, unsigned int y2, int xadj=0, int yadj=0 );
    int setCapAClip( int x, int y, unsigned int width, unsigned int height );
    void getCapAClip( int *x, int *y, unsigned int *width, unsigned int *height );
    int applyCapAClip( int nclips );
    bool changedCapAClip() { return clipTabChanged; }

    // additional
    int getWWidth() { return vwin.width; }
    int getWHeight() { return vwin.height; }

    // frame buffer
    unsigned int getFBheight() { return vbuf.height; }
    unsigned int getFBwidth() { return vbuf.width; }
    unsigned int getFBdepth() { return vbuf.depth; }
    unsigned int getFBbpl() { return vbuf.bytesperline; }

    // capture image stuff
    bool grabSetParams( int width, int height, int palette );
    char * grabCapture( bool single );

    int isOk() {return _state;}
protected:
    bool grabOne( int frame );
    void grabWait( int frame );

    // video 4 linux
    // frame buffer
    struct video_buffer vbuf;

    // capabilities
    struct video_capability vcap;
    struct video_channel *vchan;
    struct video_tuner vtuner;
    struct video_picture vpic;
    struct video_audio *vaudio;

    // grabbing stuff
    struct video_mmap vgrab[2];
    struct video_mbuf vmbuf;
    char * grabbermem;                   // memory for snapshot  (was: unchar)
    int msize;                           // size of memory for grabbermem

    // capture area stuff
    struct video_window vwin;
    struct video_clip cliprecs[MAXCLIPRECS];    // list of clips
    int nrofclips;

    // additional
    int achan;  // the active channel set by setChannel()
    int aaudio; // the active audio set by setAudio()
    int atuner; // active tuner set by setTuner()

    bool scapture, smute;

    int gsync,ggrab;
    bool geven;

    bool setupok;

    bool clipTabChanged;

    int _state;
};

class v4l1if : public v4l1baseif {
public:
    v4l1if( const char* mem=0, const char * _device = V4L_DEVICE, int _bpp = 0, int _palette = 0 );

    virtual void setPalette( int pal );

private:
    unsigned int pixmap_bytes,x11_format;

};

#endif
