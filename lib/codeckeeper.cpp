#include "avm_cpuinfo.h"
#include "avm_except.h" // catch only
#include "avm_creators.h"
#include "videodecoder.h"
#include "videoencoder.h"
#include "audiodecoder.h"
#include "audioencoder.h"
#include "plugin.h"
#include "configfile.h"
#include "utils.h"
#include "avm_output.h"

#include "version.h"

#include "../plugins/libac3pass/fillplugins.h"
#include "../plugins/libaudiodec/fillplugins.h"
#include "../plugins/libdivx4/fillplugins.h"
#include "../plugins/libffmpeg/fillplugins.h"
#include "../plugins/libmad/fillplugins.h"
#include "../plugins/libmp3lame_audioenc/fillplugins.h"
#include "../plugins/libmp3lamebin_audioenc/fillplugins.h"
#include "../plugins/libmpeg_audiodec/fillplugins.h"
#include "../plugins/libvorbis/fillplugins.h"
#include "../plugins/libwin32/fillplugins.h"
#include "../plugins/libxvid/fillplugins.h"
#include "../plugins/libxvid4/fillplugins.h"
#include "../samples/mjpeg_plugin/fillplugins.h"
#include "Uncompressed.h"

#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>



#ifndef	RTLD_PARENT
#define	RTLD_PARENT 0
#endif

#if 1
// gdb catch point
extern "C" void trapbug(void)
{
    static int a = 0;
    a++;
}
#endif

avm::vector<avm::CodecInfo> video_codecs;
avm::vector<avm::CodecInfo> audio_codecs;

#define Debug if(0)
static bool pluginit = false;

const char* plugin_def_path = PLUGIN_PATH;
static avm::string last_error;


AVM_BEGIN_NAMESPACE;

static avm::vector<CodecInfo*> video_order;
static avm::vector<CodecInfo*> audio_order;

#ifdef ARCH_X86
/*
 * LDT_Keeper MUST be here
 * LDT must be modified before program creates first thread
 */

// including c-file is slightly unusual, but
// we prevent to have duplicated code
#include "../plugins/libwin32/loader/ldt_keeper.c"

struct LDT_Keeper
{
    ldt_fs_t* ldt_fs;

    LDT_Keeper() : ldt_fs(0) {}
    ~LDT_Keeper() { if (ldt_fs) Restore_LDT_Keeper(ldt_fs); }
    // to be initialized only when Win32 plugin is available
    void init()
    {
	if (!ldt_fs)
	{
	    ldt_fs = Setup_LDT_Keeper();
	    if (ldt_fs)
		AVM_WRITE("LDT keeper", "Installed fs segment: %p\n", ldt_fs->fs_seg);
	}
    }
};

static LDT_Keeper Keeper;
#endif

static void plugin_close(const CodecInfo& ci)
{
    if (ci.handle)
    {
	PluginPrivate* pi = (PluginPrivate*) ci.handle;

	//printf("PLUGINCLOSE  %p   c:%d\n", pi, (pi)?pi->refcount:12345);
	if (--pi->refcount <= 0 && pi->dlhandle)
	{
	    if (((codec_plugin_t*)pi->fchandle)
		&& ((codec_plugin_t*)pi->fchandle)->error)
	    {
		free(((codec_plugin_t*)pi->fchandle)->error);
		((codec_plugin_t*)pi->fchandle)->error = 0;
	    }
	    dlclose(pi->dlhandle);
	    delete pi;
	    ci.handle = 0;
	}
    }
}

codec_plugin_t* plugin_open(const CodecInfo& ci)
{
    PluginPrivate* pi;
    const char* name = ci.modulename.c_str();
    if (!ci.handle)
    {
	pi = new PluginPrivate;
	pi->dlhandle = dlopen(name, RTLD_LAZY | RTLD_PARENT);
	if (!pi->dlhandle)
	{
	    AVM_WRITE("codec keeper", "WARNING: plugin %s could not be opened: %s\n",
		      name, dlerror());
	    delete pi;
	    return 0;
	}
	pi->fchandle = 0;
        pi->refcount = 0;
	ci.handle = pi;
    }
    else
        pi = (PluginPrivate*) ci.handle;

    pi->refcount++;
    if (!pi->fchandle)
    {
	char plgn[100];
	strcpy(plgn, "avm_codec_plugin_");
	char* x = strrchr(name, '/');
	strncat(plgn, x + 1, 50);
	x = strchr(plgn, '.');
	*x = 0;
	codec_plugin_t* plugin = (codec_plugin_t*) dlsym(pi->dlhandle, plgn);

	if (!plugin || (plugin->version != PLUGIN_API_VERSION))
	{
            if (plugin)
		AVM_WRITE("codec keeper", "WARNING: plugin %s has version %d, expected %d (should be removed)\n",
			  name, plugin->version, PLUGIN_API_VERSION);
	    else
		AVM_WRITE("codec keeper", "WARNING: plugin %s is in incompatible format\n",
			  name);
	    plugin_close(ci);
	    return 0;
	}
	pi->fchandle = plugin;
    }
    return (codec_plugin_t*) pi->fchandle;
}

static void plugin_add_list(avm::vector<CodecInfo>& ci, const avm::string& fullname)
{
    int vp = 0;
    for (unsigned i = 0; i < ci.size(); i++)
    {
	ci[i].modulename = fullname;
	if (ci[i].media == CodecInfo::Video)
	{
	    vp++;
	    video_codecs.push_back(ci[i]);
	}
	else
	    audio_codecs.push_back(ci[i]);
    }
    AVM_WRITE("codec keeper", 1, "%s:  A/V %d/%d\n",
	      fullname.c_str(), ci.size() - vp, vp);
}

static void plugin_get_error(codec_plugin_t* plugin)
{
    if (plugin && plugin->error)
	AVM_WRITE("codec keeper", "%s\n", plugin->error);
}

// we have to avoid prefixed positive matches
// i.e.  ffmpeg_divx  will not match ffmpeg
static const char* sort_str(const char* haystack, const char* needle)
{
    const char* r = haystack;
    int nl = strlen(needle);

    if (!strlen(haystack) || !nl)
	return 0;
    //printf("HAY %s  %s\n", haystack, needle);
    while ((r = strstr(r, needle)) != NULL)
    {
	//printf("r  %50s  %s  %p\n", r, haystack, needle);
	if (r == haystack || r[-1] == ',')
	{
            r += nl;
	    if (!*r || *r == ',')
		return r;
	}
	r++; // next char (also skips ',')
    }
    return 0;
}

static int sort_codec_list(avm::vector<CodecInfo*>& codecs,
			   const char* orderlist)
{
    // simple stupid bubble sort - this code is not supposed
    // to be executed very often...
    //
    // only CodecInfos' found in the list will be swaped
    // rest of them will stay in the unspecified order

    ///return 0; // disabled for now

    if (!orderlist)
	return 0;

    // always be sure list contains all codecs
    // so repeat all of them after passed parameter
    avm::string tl = orderlist;
    for (unsigned i = 0; i < codecs.size(); i++)
    {
	const char* p = sort_str(tl.c_str(), codecs[i]->GetPrivateName());
	if (!p)
	    p =  sort_str(tl.c_str(), codecs[i]->GetName());
	if (!p)
	{
	    tl += ",";
	    tl += codecs[i]->GetPrivateName();
	    //printf("MISSING %s\n", codecs[i]->GetPrivateName());
	}
    }

    orderlist = tl.c_str();
    int changed = 0;
    int len = strlen(orderlist);
    //printf("Sort for list of %d codecs with: %s  \n", codecs.size(), orderlist);
    for (unsigned i = 0; i < codecs.size(); i++)
    {
	//printf("Order %d:    %s  (%s)\n", i, codecs[i]->GetPrivateName(), codecs[i]->GetName());
	const char* p = sort_str(orderlist, codecs[i]->GetPrivateName());
	if (!p) // tagged to the first position
	    p = sort_str(orderlist, codecs[i]->GetName());
	if (!p) // tagged to the first position
	    continue;//orderlist + len; // last position
	int s = 0;
	for (unsigned j = i + 1; j < codecs.size(); j++)
	{
	    //cout << "    Compare with " << codecs[j].name.c_str() << endl;
	    const char* r = sort_str(orderlist, codecs[j]->GetPrivateName());
            if (!r)
		r = sort_str(orderlist, codecs[j]->GetName());
	    //cout << " ptr: " << (void*) r << "     p: " << (void*) p << endl;
	    if (r && r < p)
	    {
		//printf("FOUND  %.10s  %.10s  pn:%s\n", p, r, codecs[j]->GetPrivateName());
		s = j;
                p = r;
	    }
	}

	if (s > 0)
	{   
	    //printf("Swap  %s <> %s\n", codecs[s]->GetPrivateName(), codecs[i]->GetPrivateName());
            CodecInfo* t = codecs[s];
	    codecs[s] = codecs[i];
	    codecs[i] = t;
            changed++;
	}
    }
    //for (unsigned i = 0; i < codecs.size(); i++)
    //    printf("Sorted %d:  %s  (%s)\n", i, codecs[i]->GetPrivateName(), codecs[i]->GetName());
    return changed;
}

static void plugin_fill()
{
    if (pluginit)
	return;

    pluginit = true;
    video_codecs.clear();
    audio_codecs.clear();

    // FFMPEG initialization
    avcodec_init();
    avcodec_register_all();

    uncompressed_FillPlugins(video_codecs);

    if (getenv("AVIPLUGIN_PATH"))
	plugin_def_path = getenv("AVIPLUGIN_PATH");

    struct dirent *dp;
    int disc = 0;
    // for linux prefer memory leakless opendir
#if defined(HAVE_OPENDIR)
    DIR *dir = opendir(plugin_def_path);
    if (dir != NULL) while ((dp = readdir(dir)) != NULL)
    {
#elif defined(HAVE_SCANDIR)
    struct dirent **namelist = 0;
    int n;
    n = scandir(plugin_def_path, &namelist, 0, alphasort);
    if(n>0)
    while(n--)
    {
	dp = namelist[n];
#else
#error No 'opendir' nor 'scandir' present - needs to be resolved...
#endif // HAVE_SCANDIR
	char* name=dp->d_name;
	if (strlen(name)<4)
	    continue;
        // find just .so named libraries/plugins
	if (strcmp(name+strlen(name)-3, ".so"))
	    continue;
	avm::string dllname = name;
	avm::string fullname = plugin_def_path;
	fullname += "/";
	fullname += dllname;

	static const struct plugin_map {
	    const char* name;
            void (*fill)(avm::vector<CodecInfo>& ci);
	} pm[] = {
	    { "ac3pass", ac3pass_FillPlugins },
	    { "audiodec", audiodec_FillPlugins },
	    { "divx4", divx4_FillPlugins },
	    { "ffmpeg", ffmpeg_FillPlugins },
	    { "mad_audiodec", mad_FillPlugins },
	    { "mp3lame_audioenc", mp3lame_FillPlugins },
	    { "mp3lamebin_audioenc", mp3lamebin_FillPlugins },
	    { "mpeg_audiodec", mpeg_audiodec_FillPlugins },
	    { "osmjpeg", ijpg_FillPlugins },
	    { "vorbis", vorbis_FillPlugins },
	    { "win32", win32_FillPlugins },
	    { "xvid4", xvid4_FillPlugins }, // MUST BE before standalone xvid!
	    { "xvid", xvid_FillPlugins },
	    { 0 }
	};

	/**
	 * This chunk of code eliminates the need to open several shared
	 * libraries while starting every application
	 */
	const plugin_map* c = pm;
	for (; c->name; c++)
	{
	    if (dllname.substr(0, strlen(c->name)) == c->name)
	    {
		avm::vector<CodecInfo> ci;
		c->fill(ci);
		plugin_add_list(ci, fullname);
#ifdef ARCH_X86
		if (strcmp(c->name, "win32") == 0)
		    Keeper.init();
#endif
		break;
	    }
	}
        disc++;
	if (c->name)
            continue; // found in the table above

        //cout << "Loading " << fullname.c_str() << endl;
	// third party plugin
#if 0
        // FIXME
	codec_plugin_t* plugin = plugin_open(fullname.c_str());
	if (plugin && plugin->register_codecs)
	{
	    avm::vector<CodecInfo> ci;
	    plugin->register_codecs(ci);
	    PluginAddList(ci, fullname);
	}

	plugin_close(dlclose(handle);
#endif
#if defined(HAVE_SCANDIR) && !defined(HAVE_OPENDIR)
	free(dp);
#endif
    }

#if defined(HAVE_OPENDIR)
    if (dir)
	closedir(dir);
#elif defined(HAVE_SCANDIR)
    if (namelist)
	free(namelist);
#endif

    AVM_WRITE("codec keeper", "Found %d plugins (%s,A:%d,V:%d)\n",
	      disc, plugin_def_path, audio_codecs.size(), video_codecs.size());
    // hack
    avm::string sol;

    // move just numbers...
    video_order.resize(video_codecs.size());
    for (uint_t v = 0; v < video_order.size(); v++) video_order[v] = &video_codecs[v];
    audio_order.resize(audio_codecs.size());
    for (uint_t a = 0; a < audio_order.size(); a++) audio_order[a] = &audio_codecs[a];

    // use by default - but each codec might use it's own order
    sol = RegReadString("aviplay", "codecs-video", "ff");
    sort_codec_list(video_order, sol.c_str());

    sol = RegReadString("aviplay", "codecs-audio", "ff");
    sort_codec_list(audio_order, sol.c_str());
}


avm::string CodecGetError()
{
    return last_error;
}

void CodecInfo::Get(avm::vector<const CodecInfo*>& infos, CodecInfo::Media media,
		    Direction dir, fourcc_t fcc)
{
    plugin_fill();
    avm::vector<CodecInfo*>& v = (media == Video) ? video_order : audio_order;

    //infos.clear(); // ??? unsure
    for (uint_t i = 0; i < v.size(); i++)
    {
	if (v[i]->direction != Both
	    && dir != Both
	    && dir != v[i]->direction)
	    continue;

	// check if fcc match is requested
        // for the encoding the first fcc must be matching
	if (fcc != ANY
	    && ((dir == Encode && fcc != v[i]->fourcc)
		|| v.invalid == v[i]->fourcc_array.find(fcc)))
	    continue;

	infos.push_back(v[i]);
    }
    //printf("Selected %d infos\n", infos.size());
}


IVideoDecoder* CreateDecoderVideo(const BITMAPINFOHEADER& bh, int depth, int flip, const char* privcname)
{
    plugin_fill();
    if (bh.biCompression == 0xffffffff)
        return 0;

    avm::vector<CodecInfo*>::iterator it;
    for (it = video_order.begin(); it != video_order.end(); it++)
    {
	CodecInfo& ci = *(*it);
	if (!(ci.direction & CodecInfo::Decode))
	    continue;
	if (privcname && (ci.privatename != privcname))
	    continue;
	avm::vector<fourcc_t>::const_iterator iv = ci.fourcc_array.begin();
	for (; iv != ci.fourcc_array.end(); iv++)
	{
	    if (bh.biCompression == *iv)
	    {
		codec_plugin_t* plugin;

		switch (ci.kind)
		{
		case CodecInfo::Source:
		    return new Unc_Decoder(ci, bh, flip);
		default:
		    plugin = plugin_open(ci);
		    //AVM_WRITE("CodecKeeper", "Modname %s %p\n", ci.modulename.c_str(), plugin);
		    if (plugin && plugin->video_decoder)
		    {
			IVideoDecoder* vd = plugin->video_decoder(ci, bh, flip);
			if (!vd && (fourcc_t)bh.biCompression != ci.fourcc)
			{
			    // try again with default codec for this
			    // fourcc array
			    AVM_WRITE("codec keeper", "Trying to use %.4s instead of %.4s\n",
				      (const char*)&ci.fourcc,
				      (const char*)&bh.biCompression);
			    BITMAPINFOHEADER dbh(bh);
			    dbh.biCompression = ci.fourcc;
			    vd = plugin->video_decoder(ci, dbh, flip);
			}

			if (vd)
			{
			    //it->handle = handle;
			    AVM_WRITE("codec keeper", "Created video decoder: %s\n", ci.GetName());
			    return vd;
			}
			plugin_get_error(plugin);
		    }
		    plugin_close(ci);
		}//switch
	    }//if
	}//for
    }//for

    char s[5];
    char err[100];
    sprintf(err, "Unknown codec 0x%x = \"%.4s\"", bh.biCompression,
	    avm_set_le32(s, bh.biCompression));
    last_error = err;
    AVM_WRITE("codec keeper", "CreateVideoDecoder(): %s\n", err);
    return 0;
}

IVideoEncoder* CreateEncoderVideo(const CodecInfo& ci, const BITMAPINFOHEADER& bh)
{
    if (!(ci.direction & CodecInfo::Encode))
	return 0;
    uint_t index = video_codecs.find(ci);
    if (index == avm::vector<CodecInfo>::invalid)
    {
	AVM_WRITE("codec keeper", "Failed to find this CodecInfo in list\n");
	return 0;
    }

    codec_plugin_t* plugin;
    switch(ci.kind)
    {
    case CodecInfo::Source:
	try { return new Unc_Encoder(ci, bh.biCompression, bh);
	} catch (FatalError& e) { e.PrintAll(); }
	break;
    default:
        plugin = plugin_open(ci);
	if (plugin && plugin->video_encoder)
	{
	    IVideoEncoder* ve = plugin->video_encoder(ci, ci.fourcc, bh);
	    if (ve)
	    {
		// video_codecs[index] is same as ci, the only difference is in permissions
		//video_codecs[index].handle = handle;
	        return ve;
    	    }
	    plugin_get_error(plugin);
	}
	plugin_close(ci);
    }
    return 0;
}

IVideoEncoder* CreateEncoderVideo(fourcc_t compressor, const BITMAPINFOHEADER& bh, const char* cname)
{
    plugin_fill();

    avm::vector<CodecInfo*>::iterator it;
    for (it = video_order.begin(); it != video_order.end(); it++)
    {
        CodecInfo& ci = *(*it);
	if (!(ci.direction & CodecInfo::Encode)
	    || (cname && strcmp(ci.GetName(), cname)))
	    continue;
	//AVM_WRITE("codec keeper", 0, "Check dir:%d  0x%x (%.4s)   cname:(%s)\n", ci.direction,
	//	  compressor, (const char*) &compressor, cname);
	if ((cname && !compressor) || ci.fourcc == compressor)
	{
    	    IVideoEncoder* ve = CreateEncoderVideo(ci, bh);
	    if(ve)
	    {
		AVM_WRITE("codec keeper", "%s video encoder created\n", ci.GetName());
	        return ve;
	    }
	}
    }

    char s[5];
    char err[100];
    sprintf(err, "Unknown codec 0x%x = \"%.4s\"", compressor,
	    avm_set_le32(s, compressor));
    last_error = err;
    AVM_WRITE("codec keeper", "CreateVideoEncoder(): %s\n", err);
    return 0;
}

IVideoEncoder* CreateEncoderVideo(const VideoEncoderInfo& info)
{
    const char* cname = info.cname.c_str();
    if (strlen(cname) == 0)
        cname = 0;
    IVideoEncoder* ve = CreateEncoderVideo(info.compressor, info.header, cname);

    return ve;
}

IAudioEncoder* CreateEncoderAudio(const CodecInfo& ci, const WAVEFORMATEX* fmt)
{
    if (!(ci.direction & CodecInfo::Encode))
	return 0;
    uint_t index = audio_codecs.find(ci);
    if (index == avm::vector<CodecInfo>::invalid)
    {
	AVM_WRITE("codec keeper", "Failed to find this CodecInfo in list\n");
	return 0;
    }

    codec_plugin_t* plugin = plugin_open(ci);
    if (plugin && plugin->audio_encoder)
    {
	IAudioEncoder* ae = plugin->audio_encoder(ci, ci.fourcc, fmt);
	if (ae)
	    return ae;
	plugin_get_error(plugin);
    }
    plugin_close(ci);

    return 0;
}

IAudioEncoder* CreateEncoderAudio(fourcc_t compressor, const WAVEFORMATEX* format)
{
    plugin_fill();

    avm::vector<CodecInfo*>::iterator it;
    for (it = audio_order.begin(); it != audio_order.end(); it++)
    {
	CodecInfo& ci = *(*it);
	if (!(ci.direction & CodecInfo::Encode))
	    continue;
	if(ci.fourcc_array.find(compressor) != avm::vector<fourcc_t>::invalid)
	{
	    IAudioEncoder* ae = CreateEncoderAudio(ci, format);
	    if(ae)
	        return ae;
	}
    }
    char err[100];
    sprintf(err, "No audio decoder for ID 0x%x", format->wFormatTag);
    last_error = err;
    return 0;
}

IAudioDecoder* CreateDecoderAudio(const WAVEFORMATEX* format, const char* privcname)
{
    plugin_fill();

    avm::vector<CodecInfo*>::iterator it;
    for (it = audio_order.begin(); it != audio_order.end(); it++)
    {
        CodecInfo& ci = *(*it);
	if (!(ci.direction & CodecInfo::Decode))
	    continue;
	if (privcname && (ci.privatename != privcname))
	    continue;

	avm::vector<fourcc_t>::const_iterator iv;
	for (iv = ci.fourcc_array.begin(); iv != ci.fourcc_array.end(); iv++)
	{
	    //printf("Audio search %x  for %x\n", format->wFormatTag, *iv);
	    if (unsigned(format->wFormatTag) == *iv)
	    {
		codec_plugin_t* plugin;
		switch(ci.kind)
		{
		default:
		    if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		    {
			// check GUID
			//cout << "Test audio extended header" << endl;
			const WAVEFORMATEXTENSIBLE* we = (const WAVEFORMATEXTENSIBLE*) format;
			if (memcmp(&we->SubFormat, &ci.guid, sizeof(GUID)) != 0)
			    continue;
		    }
		    plugin = plugin_open(ci);
		    if (plugin && plugin->audio_decoder)
		    {
			IAudioDecoder* ad = plugin->audio_decoder(ci, format);
			if (ad)
			{
			    //ci.handle = handle;
			    AVM_WRITE("codec keeper", "%s audio decoder created\n", ci.GetName());
			    return ad;
			}

			plugin_get_error(plugin);
		    }
		    plugin_close(ci);
		    continue;
		}
	    }
	}
    }

    char err[100];
    sprintf(err, "No audio decoder for ID 0x%x", format->wFormatTag);
    last_error = err;
    return 0;
}

float CodecGetAttr(const CodecInfo& info, const char* attribute, float* value)
{
    float result = -1;
    codec_plugin_t* plugin = plugin_open(info);

    if (plugin && plugin->get_attr_float)
	result = plugin->get_attr_float(info, attribute, value);
    plugin_close(info);

    return result;
}

int CodecSetAttr(const CodecInfo& info, const char* attribute, float value)
{
    int result = -1;
    codec_plugin_t* plugin = plugin_open(info);

    if (plugin && plugin->set_attr_float)
	result = plugin->set_attr_float(info, attribute, value);
    plugin_close(info);

    return result;
}

int CodecGetAttr(const CodecInfo& info, const char* attribute, int* value)
{
    int result = -1;
    codec_plugin_t* plugin = plugin_open(info);

    if (plugin && plugin->get_attr_int)
	result = plugin->get_attr_int(info, attribute, value);
    plugin_close(info);

    return result;
}

int CodecSetAttr(const CodecInfo& info, const char* attribute, int value)
{
    int result = -1;
    codec_plugin_t* plugin = plugin_open(info);

    if (plugin && plugin->set_attr_int)
	result = plugin->set_attr_int(info, attribute, value);
    plugin_close(info);

    return result;
}

int CodecGetAttr(const CodecInfo& info, const char* attribute, const char** value)
{
    int result = -1;
    codec_plugin_t* plugin = plugin_open(info);

    if (plugin && plugin->get_attr_string)
	result = plugin->get_attr_string(info, attribute, value);
    plugin_close(info);

    return result;
}

int CodecSetAttr(const CodecInfo& info, const char* attribute, const char* value)
{
    int result = -1;
    codec_plugin_t* plugin = plugin_open(info);
    if (plugin && plugin->set_attr_string)
	result = plugin->set_attr_string(info, attribute, value);
    plugin_close(info);
    return result;
}

void FreeDecoderAudio(IAudioDecoder* decoder)
{
    if (decoder)
    {
	const CodecInfo& info = decoder->GetCodecInfo();
	AVM_WRITE("codec keeper", 1, "FreeAudioDecoder() %s\n", info.GetName());
	delete decoder;
	plugin_close(info);
    }
}

void FreeEncoderAudio(IAudioEncoder* encoder)
{
    if (encoder)
    {
	const CodecInfo& info = encoder->GetCodecInfo();
	AVM_WRITE("codec keeper", 1, "FreeAudioEncoder() %s\n", info.GetName());
	delete encoder;
	plugin_close(info);
    }
}

void FreeDecoderVideo(IVideoDecoder* decoder)
{
    if (decoder)
    {
	const CodecInfo& info = decoder->GetCodecInfo();
	AVM_WRITE("codec keeper", 1, "FreeVideoDecoder() %s\n", info.GetName());
	delete decoder;
	plugin_close(info);
    }
}

void FreeEncoderVideo(IVideoEncoder* encoder)
{
    if (encoder)
    {
	const CodecInfo& info = encoder->GetCodecInfo();
	AVM_WRITE("codec keeper", 1, "FreeVideoEncoder() %s\n", info.GetName());
	delete encoder;
	plugin_close(info);
    }
}

int SortVideoCodecs(const char* orderlist)
{
    return sort_codec_list(video_order, orderlist);
}

int SortAudioCodecs(const char* orderlist)
{
    return sort_codec_list(audio_order, orderlist);
}

int PluginGetAttrFloat(const CodecInfo& info, const char* attribute, float* value)
{
    const AttributeInfo* attr = info.FindAttribute(attribute);
    const char* n = info.privatename.c_str();

    if (attr && attr->GetKind() == AttributeInfo::Float)
    {
	*value = RegReadFloat(n, attribute, attr->GetDefaultFloat());
	return 0;
    }
    AVM_WRITE(n, "GetAttrFloat unsupported attribute or incorrect value %s = %f\n", attribute, value);\
    return -1;
}

int PluginSetAttrFloat(const CodecInfo& info, const char* attribute, float value)
{
    const AttributeInfo* attr = info.FindAttribute(attribute);
    const char* n = info.privatename.c_str();

    if (attr && attr->GetKind() == AttributeInfo::Float
	&& attr->IsValid(value))
	return RegWriteFloat(n, attribute, value);

    AVM_WRITE(n, "SetAttrFloat unsupported attribute or incorrect value %s = %f\n", attribute, value);
    return -1;
}

int PluginGetAttrInt(const CodecInfo& info, const char* attribute, int* value)
{
    const AttributeInfo* attr = info.FindAttribute(attribute);
    const char* n = info.privatename.c_str();

    if (attr && (attr->GetKind() == AttributeInfo::Integer
		 || attr->GetKind() == AttributeInfo::Select))
    {
	*value = RegReadInt(n, attribute, attr->GetDefault());
        return 0;
    }
    AVM_WRITE(n, "GetAttrInt unsupported attribute or incorrect value %s = %d\n", attribute, value);\
    return -1;
}

int PluginSetAttrInt(const CodecInfo& info, const char* attribute, int value)
{
    const AttributeInfo* attr = info.FindAttribute(attribute);
    const avm::string n = info.privatename;

    if (attr && (attr->GetKind() == AttributeInfo::Integer
		 || attr->GetKind() == AttributeInfo::Select)
	&& attr->IsValid(value))
    {
	return RegWriteInt(n, attribute, value);
    }

    AVM_WRITE(n, "SetAttrInt unsupported attribute or incorrect value %s = %d\n", attribute, value);
    return -1;
}

int PluginGetAttrString(const CodecInfo& info, const char* attribute, const char** value)
{
    const AttributeInfo* attr = info.FindAttribute(attribute);
    const char* n = info.privatename.c_str();

    if (value && attr && (attr->GetKind() == AttributeInfo::String))
    {
	*value = RegReadString(n, attribute, "");
        return 0;
    }
    AVM_WRITE(n, "GetAttrStr unsupported attribute or incorrect value %s = %p\n", attribute, value);\
    return -1;
}

int PluginSetAttrString(const CodecInfo& info, const char* attribute, const char* value)
{
    const AttributeInfo* attr = info.FindAttribute(attribute);
    const char* n = info.privatename.c_str();

    if (value && attr && (attr->GetKind() == AttributeInfo::String))
	return RegWriteString(n, attribute, value);

    AVM_WRITE(n, "SetAttrString unsupported attribute or incorrect value %s = %p\n", attribute, value);
    return -1;
}

const CodecInfo* CodecInfo::match(fourcc_t fcccodec, CodecInfo::Media cimedia, const CodecInfo* start, CodecInfo::Direction cdir)
{
    if (video_codecs.size() == 0 && audio_codecs.size() == 0)
    {
	BITMAPINFOHEADER bi;
        bi.biCompression = 0xffffffff;
	// just to fill video_codecs list
	CreateDecoderVideo(bi, 0, 0);
    }

    avm::vector<CodecInfo>& c =
	(cimedia == Video) ? video_codecs : audio_codecs;

    for (unsigned i = 0; i < c.size(); i++)
    {
	if (start)
	{
	    if (&c[i] == start)
		start = 0;
	    continue;
	}

	CodecInfo& ci = c[i];

	if (((int)ci.direction & (int)cdir) != (int)cdir)
	    continue;

	//printf("CHECK DIR %d   %x    %x  %s\n", ci.direction, fcccodec, ci.fourcc, ci.GetName());
	if (cdir == Encode)
	{
            // for encoding we want exact match - the first fourcc
	    if (fcccodec == ci.fourcc)
		return &ci;
	}
        else
	{
	    for (unsigned j = 0; j < ci.fourcc_array.size(); j++)
	    {
		//printf("SEARCH FOR %.4s (0x%x)  %.4s (0x%x)  %d:%d\n", (char*)&fcccodec, fcccodec, (char*)&ci.fourcc_array[j], ci.fourcc_array[j], i, j);
		if (fcccodec == ci.fourcc_array[j])
		    return &ci;
	    }
	}
    }
    return 0;
}

const CodecInfo* CodecInfo::match(CodecInfo::Media cimedia, const char* pname)
{
    if (video_codecs.size() == 0 && audio_codecs.size() == 0)
    {
	BITMAPINFOHEADER bi;
        bi.biCompression = 0xffffffff;
	// just to fill video_codecs list
	CreateDecoderVideo(bi, 0, 0);
    }

    avm::vector<CodecInfo>& c = (cimedia == Video ) ? video_codecs : audio_codecs;
    for (unsigned i = 0; i < c.size(); i++)
    {
	CodecInfo& ci = c[i];

	if (strcmp(ci.GetPrivateName(), pname) == 0
	    || strcmp(ci.GetName(), pname) == 0)
	    return &ci;
    }

    return 0;
}

extern "C" int GetAvifileVersion()
{
    out.setDebugLevel(0, 0);
    return AVIFILE_VERSION;
}

AVM_END_NAMESPACE;
