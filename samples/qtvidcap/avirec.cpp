/*  avirec
 Copyright (C) 1999-2001 Oliver Kurth

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* $Id: avirec.cpp,v 1.30 2003/06/02 02:55:34 alexannika Exp $ */


#include <avifile.h>
#include <videoencoder.h>
#include <avm_creators.h>
#include <avm_except.h>
#include <avm_cpuinfo.h>

#include "v4lxif.h"
#include "capproc.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <signal.h> // sighandler_t

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include <getopt.h>

#ifdef HAVE_SYSINFO
#include <sys/sysinfo.h>
#endif


using namespace std;

// for now same name as avicap to share settings
const char* g_pcProgramName = "AviCap"; //"AviRec";
unsigned int m_iMemory;


struct VRConf
{
    avm::string codec_string;
    int attr_count;
    char *attributes[10];
    int width;
    int height;
    int over_l;
    int over_r;
    int over_t;
    int over_b;
    int quality;
    int keyframes;
    int audio_channels;
    int audio_compressor;
    int audio_bitrate;
    int timelimit;
    float fps;
    bool deinterlace;
    bool rgb;
    int norm;
    int input;
    const char *device;
    const char *audio_device;
};

static const struct STRTAB norms_bttv[] = {
    {  0, "PAL" },
    {  1, "NTSC" },
    {  2, "SECAM" },
    {  3, "PAL-NC" },
    {  4, "PAL-M" },
    {  5, "PAL-N" },
    {  6, "NTSC-JP" },
    { -1, NULL }
};


static struct VRConf conf = {
    "",
    0,
    { 0 },
    384,
    288,
    0,
    0,
    0,
    0,
    95,
    15,
    2,
    1, // PCM
    0,
    60,
    25,
    false,
    false,
    -1,
    0,
    V4L_DEVICE,
    "/dev/dsp",
};

volatile int sig_int_seen = 0;
static void sig_int(int)
{
    sig_int_seen = 1;
}

avm::vector<avm::CodecInfo>::iterator GetCodecID(const avm::string& name)
{
    avm::vector<avm::CodecInfo>::iterator it;
    int i=0;
    int sel=0;

    for (it = video_codecs.begin(); it != video_codecs.end(); it++)
    {
	if(!(it->direction & avm::CodecInfo::Encode))
	    continue;

	if(name == it->GetName()) break;
    }
    return it;
}

void ListCodecs(avm::vector<avm::CodecInfo>& codec_list)
{
    avm::vector<avm::CodecInfo>::iterator it;
    int i=0;
    int sel=0;

    for (it = codec_list.begin(); it != codec_list.end(); it++)
    {
	if ( it->kind == avm::CodecInfo::DShow_Dec)
	    continue; //probably not a usable codec..

	if(!(it->direction & avm::CodecInfo::Encode))
	    continue;

	printf("%s\n", it->GetName());

	avm::vector<avm::AttributeInfo> encinfo = it->encoder_info;
	avm::vector<avm::AttributeInfo>::const_iterator inf_it;

	for(inf_it = encinfo.begin(); inf_it != encinfo.end(); inf_it++)
	{
	    switch(inf_it->kind)
	    {
	    case avm::AttributeInfo::Integer:
		{
		    int defval;
		    if (avm::CodecGetAttr(*it, inf_it->GetName(), &defval) == 0)
			printf("  %s=%d\n", inf_it->GetName(), defval);
		    else
			printf("  %s=(no default)\n", inf_it->GetName());
		}
		break;
	    case avm::AttributeInfo::Select:
		{
		    int defval;
		    avm::vector<avm::string>::const_iterator sit;
		    avm::CodecGetAttr(*it, inf_it->GetName(), &defval);
                    printf("  %s = ", inf_it->GetName());
		    printf(" %s ", (defval < (int)inf_it->options.size()) ?
			   inf_it->options[defval].c_str() : "unknown");

		    printf(" (");
		    for (sit = (inf_it->options).begin(); sit != (inf_it->options).end(); sit++)
			printf("%s ", sit->c_str());

		    printf(")\n");
		}
		break;
	    case avm::AttributeInfo::String:
		{
		    const char* def_str;
		    avm::CodecGetAttr(*it, it->GetName(), &def_str);
		    printf(" %s = '%s'\n", inf_it->GetName(), def_str);
		}
		break;
	    }
	}

	i++;
    }
}

void Usage(const char *arg0)
{
    printf("Usage: %s [options] [filename]\n"
	   "Options:\n"
	   "opt  | long option       arg    default\n"
	   "-----+--------------------------------------------------\n"
	   "  -h | --help                        display usage\n"
	   "  -l | --list                        list available video codecs\n"
	   //"  -L | --list-audio                  list available audio codecs\n";
	   "  -a | --codec-attribute attr        set a codec attribute\n"
	   "  -b | --audiobitrate    rate     0  set audio bitrate (for mp3)\n"
	   "  -c | --codec           codec       set video codec\n"
	   "  -d | --deinterlace            off  deinterlace picture\n"
	   "  -F | --fps             rate    25  set framerate\n"
	   "  -g | --grabdevice      device "V4L_DEVICE" set video device\n"
	   "  -G | --audiodevice      device /dev/dsp set audio device\n"
	   "  -i | --input           input    0  set v4l input\n"
	   "  -k | --keyframes       rate    15  set key frame rate\n"
	   "  -m | --audiomode       val      s  audio mode (n=none, m=mono, s=stereo)\n"
	   "  -n | --norm            norm (none) set tuner norm (pal,ntsc,secam,pal-nc,pal-m,ntsc-jp)\n"
	   "  -q | --quality         val     95  set quality 1..100\n"
	   "  -r | --rgb                    off  capture in rgb color space\n"
	   "  -t | --rectime         time   60s  set time limit (s,m,h,d,w)\n"
	   "  -x | --width           cols   384  set capture width\n"
	   "  -y | --height          rows   288  set capture height\n"
	   "  -L | --over-l          cols   0    set left overscan\n"
	   "  -R | --over-r          cols   0    set right overscan\n"
	   "  -T | --over-t          rows   0    set top overscan\n"
	   "  -B | --over-b          rows   0    set bottom overscan\n"
	   , arg0);
}

int time_interval(char *str)
{
    char buf[16];
    char *p = str, *q = buf;
    int factor = 1, val;

    while(*p && isdigit(*p))
	*(q++) = *(p++);

    *q = 0;
    val = atoi(buf);

    /* fall through: */
    switch(*p)
    {
    case 'w':
	factor *= 7;
    case 'd':
	factor *=24;
    case 'h':
	factor *= 60;
    case 'm':
	factor *= 60;
    case 's':
    case 0:
	break;
    default:
	return -1;
    }
    return val * factor;
}

static  void iMemory()
{
#ifdef HAVE_SYSINFO
    struct sysinfo s_info;
    sysinfo(&s_info);
    m_iMemory=s_info.totalram;
    if(m_iMemory<2*1048576)
	m_iMemory=1048576;
    else
	m_iMemory/=2;
#else
    m_iMemory = 16 * 1024 * 1024;
#endif
    printf("Using %d Mb of memory for frame caching\n", m_iMemory/1048576);
}

struct CaptureConfig *CreateCapConf(VRConf *pconf, const avm::string& filename)
{
    CaptureConfig *pcc = new CaptureConfig;

    if(pcc)
    {
	pcc->filename = filename;
	pcc->segment_size = -1;

	avm::vector<avm::CodecInfo>::iterator it = GetCodecID(pconf->codec_string);
	if(it != video_codecs.end())
	{
	    pcc->codec.compressor = it->fourcc;
	    pcc->codec.cname=it->GetName();
	    printf("using codec '%s' (%u)\n", it->GetName(), it->fourcc);
	}
	else
	{
	    fprintf(stderr, "invalid codec: %s\nuse -l for a list\n", pconf->codec_string.c_str());
	    return NULL;
	}

	int i;
	for(i = 0; i < conf.attr_count; i++)
	{
	    char name[21], val[21];
	    sscanf(conf.attributes[i], "%20[^=]=%s", name, val);
	    printf("setting '%s' = '%s'\n", name, val);
	    avm::CodecSetAttr(*it, name, atoi(val)); // FIXME: do not assume integer attribute...
	}

	pcc->codec.quality = pconf->quality * 100;
	pcc->codec.keyfreq = pconf->keyframes;
	pcc->frequency = 44100;  // make configurable

	pcc->chan = pconf->audio_channels;

	if(pcc->chan > 0)
	{
	    if(pconf->audio_bitrate)
	    {
		pcc->audio_bitrate = pconf->audio_bitrate;
		pcc->audio_compressor = 0x55; // MP3
	    }
	    else
		pcc->audio_compressor = 1; // PCM
	}else
	    pcc->frequency = 0;  // tells CaptureProcess to not use audio

	pcc->samplesize = 16;    // make configurable
	pcc->res_w = pconf->width;
	pcc->res_h = pconf->height;

	pcc->over_l = pconf->over_l;
	pcc->over_r = pconf->over_r;
	pcc->over_t = pconf->over_t;
	pcc->over_b = pconf->over_b;

	pcc->timelimit = pconf->timelimit;
	pcc->sizelimit = -1;  // make configurable
	pcc->fps = pconf->fps;
	if(pconf->rgb)
	    pcc->colorspace = cspRGB24;
	else
	    pcc->colorspace = cspYUY2;
	pcc->deinterlace = pconf->deinterlace;
	pcc->audio_device = pconf->audio_device;
    }
    return pcc;
}

void ReadEnv()
{
    char *val;

    val = getenv("AVIREC_CODEC");
    if (val) conf.codec_string = val;
    val = getenv("AVIREC_ATTRIBUTES");
    if (val) conf.attributes[conf.attr_count++] = strdup(val);
    val = getenv("AVIREC_WIDTH");
    if (val) conf.width = atoi(val);
    val = getenv("AVIREC_HEIGHT");
    if (val) conf.height = atoi(val);
    val = getenv("AVIREC_QUALITY");
    if (val) conf.quality = atoi(val);
    val = getenv("AVIREC_KEYFRAMES");
    if (val) conf.keyframes = atoi(val);

    val = getenv("AVIREC_AUDIOBITRATE");
    if (val) conf.audio_bitrate = atoi(val);
    val = getenv("AVIREC_TIMELIMIT");
    if (val) conf.timelimit = atoi(val);
    val = getenv("AVIREC_FPS");
    if (val) conf.fps = atof(val);

    val = getenv("AVIREC_DEINTERLACE");
    if (val && strncasecmp(val, "true", 4) == 0) conf.deinterlace = true;

    val = getenv("AVIREC_RGB");
    if (val && strncasecmp(val, "true", 4) == 0) conf.rgb = true;


    val = getenv("AVIREC_NORM");
    if(val)
    {
	int i = 0;
	while(norms_bttv[i].str){
	    if(strcasecmp(norms_bttv[i].str, val) == 0) break;
	    i++;
	}
	if(norms_bttv[i].str)
	    conf.norm = norms_bttv[i].nr;
    }
    val = getenv("AVIREC_DEVICE");
    if (val) conf.device = strdup(getenv("AVIREC_DEVICE"));

    val = getenv("AVIREC_AUDIODEVICE");
    if (val) conf.audio_device = strdup(getenv("AVIREC_AUDIODEVICE"));
}

int main(int argc, char *argv[])
{
    int c;
    int digit_optind = 0;
    int do_rec = 1;

    {
	BITMAPINFOHEADER bih;
	bih.biCompression = 0xffffffff;
	// just to fill video_codecs list
	avm::CreateDecoderVideo(bih, 0, 0);
    }

    ReadEnv();

    while (1)
    {
	static const struct option long_options[] =
	{
	    {"codec-attribute", 0, NULL, 'a'},
	    {"audiobitrate", 0, NULL, 'b'},
	    {"codec",        0, NULL, 'c'},
	    {"deinterlace",  0, NULL, 'd'},
	    {"framerate",    0, NULL, 'F'},
	    {"fps",          0, NULL, 'F'},
	    {"grabdevice",   0, NULL, 'g'},
	    {"audiodevice",  0, NULL, 'G'},
	    {"help",         0, NULL, 'h'},
	    {"input",        0, NULL, 'i'},
	    {"keyframes",    0, NULL, 'k'},
	    {"list",         0, NULL, 'l'},
	    {"list-audio",   0, NULL, 'C'},
	    {"audiomode",    0, NULL, 'm'},
	    {"quality",      0, NULL, 'q'},
	    {"rgb",          0, NULL, 'r'},
	    {"rectime",      0, NULL, 't'},
	    {"width",        0, NULL, 'x'},
	    {"height",       0, NULL, 'y'}, // -h already given for help...
	    {"over-l",       0, NULL, 'L'},
	    {"over-r",       0, NULL, 'R'},
	    {"over-t",       0, NULL, 'T'},
	    {"over-b",       0, NULL, 'B'},

	    {NULL,           0, NULL,  0 }
	};
	int this_option_optind = optind ? optind : 1;
	int option_index = 0;

	c = getopt_long (argc, argv, "a:b:Cc:dF:g:G:hi:k:lm:n:q:rt:x:y:L:R:T:B:",
			 long_options, &option_index);
	if (c == -1)
	    break;

	switch (c)
	{
	case 'a':
	    conf.attributes[conf.attr_count++] = strdup(optarg);
	    break;
	case 'b':
	    conf.audio_bitrate = atoi(optarg);
	    break;
	case 'c':
	    conf.codec_string = optarg;
	    break;
	case 'd':
	    conf.deinterlace = true;
	    break;
	case 'F':
	    conf.fps = atof(optarg);
	    break;
	case 'g':
	    conf.device = strdup(optarg);
	    break;
	case 'G':
	    conf.audio_device = strdup(optarg);
	    break;
	case '?':
	case 'h':
	    Usage(argv[0]);
	    do_rec = 0;
	    break;
	case 'k':
	    conf.keyframes = atoi(optarg);
	    break;
	case 'l':
	    ListCodecs(video_codecs);
	    do_rec = 0;
	    break;
	case 'C':
	    ListCodecs(audio_codecs);
	    do_rec = 0;
	    break;
	case 'm':
	    switch (optarg[0]){
	    case 'n':
		conf.audio_channels = 0;
		break;
	    case 'm':
		conf.audio_channels = 1;
		break;
	    case 's':
		conf.audio_channels = 2;
		break;
	    }
	    break;
	case 'n':
	    {
		int i = 0;
		while(norms_bttv[i].str)
		{
		    if(strcasecmp(norms_bttv[i].str, optarg) == 0) break;
		    i++;
		}
		if(norms_bttv[i].str)
		    conf.norm = norms_bttv[i].nr;
	    }
	    break;
	case 'i':
	    conf.input = atoi(optarg);
	    break;
	case 'q':
	    conf.quality = atoi(optarg);
	    break;
	case 'r':
	    conf.rgb = true;
	    break;
	case 't':
	    conf.timelimit = time_interval(optarg);
	    break;
	case 'x':
	    conf.width = atoi(optarg);
	    break;
	case 'y':
	    conf.height = atoi(optarg);
	    break;
	case 'L':
	    conf.over_l = atoi(optarg);
	    break;
	case 'R':
	    conf.over_r = atoi(optarg);
	    break;
	case 'T':
	    conf.over_t = atoi(optarg);
	    break;
	case 'B':
	    conf.over_b = atoi(optarg);
	    break;
	}
    }

    if (optind < argc)
    {
	if(do_rec)
	{
	    iMemory();

	    avm::string filename = argv[optind++];
	    CaptureConfig *capconf = CreateCapConf(&conf, filename);

	    if(!capconf) exit(EXIT_FAILURE);

	    v4lxif* v4l;
	    try
	    {
		v4l = new v4l1baseif(0, conf.device);

		if(conf.audio_channels > 0)
		{
		    v4l->setAudioMode(conf.audio_channels == 2 ? VIDEO_SOUND_STEREO : VIDEO_SOUND_MONO);
		    v4l->setAudioMute(false);
		    v4l->setAudioVolume(65535);
		}else
		    v4l->setAudioMute(true);

		if(conf.norm > 0)
		{
		    // twice? don't ask me why...
		    v4l->setChannelNorm(conf.norm);
		    v4l->setChannelNorm(conf.norm);
		}
		if(conf.input > 0)
		    v4l->setChannel(conf.input);
	    }
	    catch(BaseError &e)
	    {
		e.Print();
		exit(EXIT_FAILURE);
	    }

	    CaptureProcess *capproc;
	    try
	    {
		capproc = new CaptureProcess(v4l, capconf, NULL);
	    }
	    catch(FatalError& e)
	    {
		fprintf(stderr, "Error: %s\n", e.GetDesc());
		exit(EXIT_FAILURE);
	    }

	    sighandler_t old_int = signal(SIGINT, sig_int);

	    CaptureProgress *progress = new CaptureProgress();
	    capproc->setMessenger(progress);

	    // now sleep until it's over...

	    int i = 0;
	    while(1)
	    {
		int finished;
		avm::string error;
		capproc->getState(finished, error);

		if(finished) break;
		if(sig_int_seen)
		{
		    fprintf(stderr, "Ctrl-C from keyboard -- please wait\n");
		    break;
		}

		if(isatty(1))
		{
		    progress->lock();
		    int drop_enc = progress->dropped_enc;
		    int drop_cap = progress->dropped_cap;
		    progress->unlock();

		    float elapsed_float=progress->enc_time/freq/1000.0;

		    printf("\r time: %d frames: %d cap drop: %d enc drop: %d",
			   (int)elapsed_float,
			   progress->video_frames,
			   progress->dropped_cap,
			   progress->dropped_enc);
		    fflush(stdout);
		}

		sleep(1);
		i++;
	    }
	    delete capproc;

	    signal(SIGINT, old_int);

	    if(conf.audio_channels > 0)
	    {
		v4l->setAudioMute(true);
	    }
	}
    }
}
