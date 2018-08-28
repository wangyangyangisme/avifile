#ifndef AVIFILE_PLUGIN_H
#define AVIFILE_PLUGIN_H

#include "infotypes.h"
#include "formats.h"

/* this is internal header for avifile */

#define PLUGIN_API_VERSION 30318

#define PLUGIN_TEMP(name) \
extern "C" avm::codec_plugin_t avm_codec_plugin_ ## name ; \
\
static inline void name ## _error_set (const char* s, ...) \
{ \
    if (avm_codec_plugin_ ## name .error) \
	free(avm_codec_plugin_ ## name .error); \
    if (s) { \
	va_list va; \
	va_start(va, s); \
        avm_codec_plugin_ ## name .error = (char*) malloc(1024); \
        strcpy(avm_codec_plugin_ ## name .error, #name);\
	int l = strlen(avm_codec_plugin_ ## name .error); \
        avm_codec_plugin_ ## name .error[l++] = ' '; \
	vsprintf(avm_codec_plugin_ ## name .error + l, s, va); \
	va_end(va); \
    } else \
        avm_codec_plugin_ ## name .error = 0; \
}

AVM_BEGIN_NAMESPACE;


class IAudioEncoder;
class IAudioDecoder;
class IVideoEncoder;
class IVideoDecoder;

/*
 * these are some commonly usable functions for avifile plugins
 * userspace apps are using CodecGet/SetAttrInt
 */
int PluginGetAttrFloat(const CodecInfo& info, const char* attribute, float* value);
int PluginSetAttrFloat(const CodecInfo& info, const char* attribute, float value);
int PluginGetAttrInt(const CodecInfo& info, const char* attribute, int* value);
int PluginSetAttrInt(const CodecInfo& info, const char* attribute, int value);
int PluginGetAttrString(const CodecInfo& info, const char* attribute, const char** value);
int PluginSetAttrString(const CodecInfo& info, const char* attribute, const char* value);


/*
 * This library is capable of using external ELF plugins for audio/video
 * (de)compression. This document describes the interface of such plugin.
 * Each plugin provides named structure equals to plugin name
 * with function table
 *
 *
 * This is private header for avifile project - user program is not
 * supposed to use this proprietary interface
 */

typedef struct {
    /*
     * Value identifies date of the last change in its API. For example,
     * value 10120 corresponds to January 20, 2001.
     */
    int version;
    char* error; // last error
    /*
     * Names of attributes and their acceptable values can be found out from
     * CodecInfo structure.
     */
    int (*get_attr_float)(const CodecInfo& info, const char* attribute, float* value);
    int (*set_attr_float)(const CodecInfo& info, const char* attribute, float value);
    int (*get_attr_int)(const CodecInfo& info, const char* attribute, int* value);
    int (*set_attr_int)(const CodecInfo& info, const char* attribute, int value);
    int (*get_attr_string)(const CodecInfo& info, const char* attribute, const char** value);
    int (*set_attr_string)(const CodecInfo& info, const char* attribute, const char* value);

    /*
     * which is used by the library to register formats supported by
     * this plugin. This function returns reference to array of CodecInfo
     * objects. Each of them refers to one supported format. See declaration
     * of CodecInfo type ( include/infotypes.h ) for details.
     *
     * When plugin supports decoding of at least one video format, it provides
     * function
     */
    void (*register_codecs)(avm::vector<CodecInfo>& codecs);
    IAudioDecoder* (*audio_decoder)(const CodecInfo& info, const WAVEFORMATEX* format);
    IAudioEncoder* (*audio_encoder)(const CodecInfo& info, fourcc_t compressor, const WAVEFORMATEX* fmt);
    IVideoDecoder* (*video_decoder)(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip);
    IVideoEncoder* (*video_encoder)(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh);

} codec_plugin_t;

struct PluginPrivate
{
    void* dlhandle;
    void* fchandle;
    int refcount;
};

AVM_END_NAMESPACE;

#endif // AVIFILE_PLUGIN_H
