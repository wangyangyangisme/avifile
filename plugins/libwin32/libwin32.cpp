#include "ACM_AudioDecoder.h"
#include "DMO_AudioDecoder.h"
#include "DS_AudioDecoder.h"
#include "VideoDecoder.h"
#include "DMO_VideoDecoder.h"
#include "DS_VideoDecoder.h"
#include "VideoEncoder.h"
#include "guids.h"
#include "registry.h"
#include "avm_output.h"
#include "configfile.h"
#include "fillplugins.h"
#include "plugin.h"

#include "loader.h"  // loader has to come first - some compilers have problems here

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

extern char* def_path;

AVM_BEGIN_NAMESPACE;

PLUGIN_TEMP(win32);

// sregistry entries
static const char* crapname = "SOFTWARE\\Microcrap\\Scrunch";
static const char* softname = "SOFTWARE\\Microsoft\\Scrunch";
static const char* videoname = "SOFTWARE\\Microsoft\\Scrunch\\Video";
static const char* loadername = "Software\\LinuxLoader\\";
static const char* divxname = "Software\\DivXNetworks\\DivX4Windows";
static const char* indeo4name = "Software\\Intel\\Indeo\\4.1";
static const char* indeo5name = "Software\\Intel\\Indeo\\5.0";
static const char* vp31name = "SOFTWARE\\ON2\\VFW Encoder/Decoder\\VP31";
static const char* xvidname = "Software\\GNU\\XviD";

static int win32_GetRegValue(const char* keyname, const char* attribute, int fccHandler, int* value, int def);

static IVideoEncoder* win32_CreateVideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
{
    VideoEncoder* e = new VideoEncoder(info, compressor, bh);
    if (e->init() == 0)
	return e;
    delete e;

    return 0;
}

static IVideoDecoder* win32_CreateVideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
{
    if (info.kind == CodecInfo::DMO)
    {
	DMO_VideoDecoder* d = new DMO_VideoDecoder(info, bh, flip);
	if (d->init() == 0)
	    return d;
	//win32_error_set(d->getError());
        delete d;
    }
    else if (info.kind == CodecInfo::DShow_Dec)
    {
	DS_VideoDecoder* d = new DS_VideoDecoder(info, bh, flip);
	if (d->init() == 0)
	{
	    if (info.fourcc == fccIV50)
	    {
                // restore INDEO settings
		const char* at[] = { "Saturation", "Brightness", "Contrast" };
		for (int i = 0; i < 3; i++)
		{
                    int value;
		    win32_GetRegValue(indeo5name, at[i], 0, &value, 0);
		    d->SetValue(at[i], value);
		}
	    }
	    return d;
	}
	//win32_error_set(d->getError());
	delete d;
    }
    else
    {
	VideoDecoder* d = new VideoDecoder(info, bh, flip);
	if (d->init() == 0)
	    return d;
	//win32_error_set(d->getError());
	delete d;
    }
    return 0;
}

static IAudioDecoder* win32_CreateAudioDecoder(const CodecInfo& info, const WAVEFORMATEX* fmt)
{
    if (info.kind == CodecInfo::DMO)
    {
	DMO_AudioDecoder* d = new DMO_AudioDecoder(info, fmt);
	if (d->init() == 0)
	    return d;
	win32_error_set(d->getError());
	delete d;
    }
    else if (info.kind == CodecInfo::DShow_Dec)
    {
	DS_AudioDecoder* d = new DS_AudioDecoder(info, fmt);
	if (d->init() == 0)
	    return d;
	win32_error_set(d->getError());
	delete d;
    }
    else
    {
	ACM_AudioDecoder* d = new ACM_AudioDecoder(info, fmt);
	if (d->init() == 0)
	    return d;
	win32_error_set(d->getError());
	delete d;
    }
    return 0;
}

/*
 * Here we've got attribute setting functions
 */
#define HKEY_CURRENT_USER       (0x80000001)
#if 0
#define HKEY_CLASSES_ROOT       (0x80000000)
#define HKEY_LOCAL_MACHINE      (0x80000002)
#define HKEY_USERS              (0x80000003)
#define HKEY_PERFORMANCE_DATA   (0x80000004)
#define HKEY_CURRENT_CONFIG     (0x80000005)
#define HKEY_DYN_DATA           (0x80000006)
#define REG_DWORD		4	/* DWORD in little endian format */
#endif

static char* win32_GetKeyName(char* s, const char* n, fourcc_t fcc)
{
    int i = strlen(n);

    strcpy(s, n);
    s[i++] = tolower(fcc & 0xff);
    fcc >>=8;
    s[i++] = tolower(fcc & 0xff);
    fcc >>=8;
    s[i++] = tolower(fcc & 0xff);
    fcc >>=8;
    s[i++] = tolower(fcc & 0xff);
    s[i] = 0;

    return s;
}

static int win32_SetRegValue(const char* keyname, const char* attribute, int value, int fccHandler)
{
    int result, status, newkey;
    char full_name[100];
    const char *k = keyname;

    if (fccHandler)
	k = win32_GetKeyName(full_name, keyname, fccHandler);
    //printf("win32setregvalue: %s\n",k);

    result = RegCreateKeyExA(HKEY_CURRENT_USER, k,
			     0, 0, 0, 0, 0, &newkey, &status);
    if (result != 0)
    {
	AVM_WRITE("Win32 plugin", "win32_SetRegValue: registry failure\n");
	return -1;
    }

    //AVM_WRITE("Win32 plugin", "KEY: %s  %s\n", named_key, attribute);
    result = RegSetValueExA(newkey, attribute, 0, REG_DWORD, &value, 4);
    if (result != 0)
    {
	AVM_WRITE("Win32 plugin", "win32_SetRegValue: error writing value\n");
    }
    // special registry for DivX codec
    if (result == 0 && strstr(attribute, "ost Process Mode") != 0)
    {
        value = -1;
	result=RegSetValueExA(newkey, "Force Post Process Mode", 0, REG_DWORD, &value, 4);
	if (result != 0)
	{
	    AVM_WRITE("Win32 plugin", "win32_SetRegValue: error writing value\n");
	}
    }
    RegCloseKey(newkey);

    //AVM_WRITE("Win32 plugin", "*********SetregFullname %s %s   %d\n", k, attribute, value);
    return result;
}

static int win32_GetRegValue(const char* keyname, const char* attribute, int fccHandler, int* value, int def)
{
    int result, status, newkey, count = 4;
    char full_name[100];
    const char *k = keyname;

    if (fccHandler)
    {
	k = win32_GetKeyName(full_name, keyname, fccHandler);
	result = RegOpenKeyExA(HKEY_CURRENT_USER, k, 0, 0, &newkey);
    }
    else
	result = RegCreateKeyExA(HKEY_CURRENT_USER, keyname,
				 0, 0, 0, 0, 0, &newkey, &status);
    //printf("win32getregvalue: %s\n",k);

    if (result != 0)
    {
	AVM_WRITE("Win32 plugin", "win32_GetRegValue: registry failure\n");
	return -1;
    }
    result = RegQueryValueExA(newkey, attribute, 0, &status, value, &count);
    if (count != 4)
	result = -1;
    RegCloseKey(newkey);
    if (result != 0)
    {
	AVM_WRITE("Win32 plugin", 1, "win32_GetRegValue: no such value for %s %s\n", keyname, attribute);
	*value = def;
        result = 0;
    }

    //AVM_WRITE("Win32 plugin", "*********GetregFullname %s %s  %d\n", k, attribute, *value);
    return result;
}

static int win32_GetAttrInt(const CodecInfo& info, const char* attribute, int* value)
{
    int result;
    switch (info.fourcc)
    {
    case fccDIVX:
	if (strcmp(attribute, "Saturation") == 0
	    || strcmp(attribute, "Brightness") == 0
	    || strcmp(attribute, "Contrast") == 0)
	    return win32_GetRegValue(divxname, attribute, 0, value, 50);
	if (strcmp(attribute + 1, "ostprocessing") == 0)
	{
	    result = win32_GetRegValue(divxname, "Postprocessing", 0, value, 30);
	    *value /= 10;
	    return result;
	}
	if (strcmp(attribute, "maxauto") == 0)
	{
	    *value = RegReadInt("win32DivX4", "maxauto", 6);
	    return 0;
	}
        break;
    case fccDIV3:
    case fccDIV4:
    case fccDIV5:
    case fccDIV6:
    case fccMP42:
	if ((strcmp(attribute, "Crispness") == 0)
	    || (strcmp(attribute, "KeyFrames") == 0)
	    || (strcmp(attribute, "BitRate") == 0))
	{
	    return win32_GetRegValue(loadername, attribute, info.fourcc, value, 0);
	}
	/* Fall through */
    case fccWMV1:
    case fccWMV2:
    case RIFFINFO_WMV3:
	if (strcmp(attribute + 1, "ostprocessing") == 0
	    ||strcmp(attribute, "Quality") == 0)
	{
	    return win32_GetRegValue((info.kind == CodecInfo::Win32)
				     ? crapname : softname,
				     "Current Post Process Mode",
				     0, value, 0);
	    //AVM_WRITE("Win32 plugin", "GETQUALITY %d\n", value);
	}
	if ((strcmp(attribute, "Saturation") == 0)
	    || (strcmp(attribute, "Hue") == 0)
	    || (strcmp(attribute, "Contrast") == 0)
	    || (strcmp(attribute, "Brightness") == 0))
	    return win32_GetRegValue(videoname, attribute, 0, value, 50);
	if (strcmp(attribute, "maxauto") == 0)
	{
	    *value = RegReadInt("win32", "maxauto", 4);
	    return 0;
	}
	if (info.FindAttribute(attribute))
	    return win32_GetRegValue(softname, attribute, 0, value, info.FindAttribute(attribute)->GetDefault());
	break;
    case fccIV31:
    case fccIV32:
    case fccIV41:
    case fccIV50:
	if (strcmp(attribute, "QuickCompress") == 0
            || strcmp(attribute, "Transparency") == 0
            || strcmp(attribute, "Scalability") == 0
            || strcmp(attribute, "Saturation") == 0
	    || strcmp(attribute, "Brightness") == 0
	    || strcmp(attribute, "Contrast") == 0)
	    return win32_GetRegValue((info.fourcc == fccIV50)
				     ? indeo5name : indeo4name,
				     attribute, 0, value, 0);
	break;
    case fccVP30:
    case fccVP31:
	if (strcmp(attribute, "strPostProcessingLevel") == 0
	    || strcmp(attribute, "strSettings") == 0)
	    return win32_GetRegValue(vp31name, attribute, 0, value, 0);
        break;
    case fccXVID:
	return win32_GetRegValue(xvidname, attribute, 0, value, 0);
    case fccMJPG:
	if (strcmp(attribute, "Mode") == 0)
	{
	    *value = _GetPrivateProfileIntA("Compress", attribute, 1, "M3JPEG.INI");
	    return 0;
	}
	break;
    }
    if (strcmp(attribute, "maxauto") == 0)
    {
	*value = 0;
	return 0;
    }
    AVM_WRITE("Win32 plugin", "GetAttrInt(): unknown attribute '%s' for codec %s\n",
	      attribute, (const char*) info.dll);
    return -1;
}

static int win32_SetAttrInt(const CodecInfo& info, const char* attribute, int value)
{
    int result;
    switch (info.fourcc)
    {
    case fccDIVX:
	if (strcmp(attribute, "Saturation") == 0
	    || strcmp(attribute, "Brightness") == 0
	    || strcmp(attribute, "Contrast") == 0)
	    return win32_SetRegValue(divxname, attribute, value, 0);
	if (strcmp(attribute, "postprocessing") == 0)
	    return win32_SetRegValue(divxname, "Postprocessing", value * 10, 0);
	if (strcmp(attribute, "maxauto") == 0)
	{
	    RegWriteInt("win32DivX4", "maxauto", value);
	    return 0;
	}
        break;
    case fccDIV3:
    case fccDIV4:
    case fccDIV5:
    case fccDIV6:
    case fccMP42:
	if ((strcmp(attribute, "Crispness") == 0)
	    || (strcmp(attribute, "KeyFrames") == 0)
	    || (strcmp(attribute, "BitRate") == 0))
	    return win32_SetRegValue(loadername, attribute, value, info.fourcc);
        /* fall through */
    case fccWMV1:
    case fccWMV2:
    case RIFFINFO_WMV3:
	if (strcmp(attribute, "postprocessing") == 0
	    || strcmp(attribute, "Quality") == 0)
	{
	    //AVM_WRITE("Win32 plugin", "SETQUALITY %d\n", value);
	    return win32_SetRegValue((info.kind == CodecInfo::Win32)
				     ? crapname : softname,
				     "Current Post Process Mode",
				     value, 0);
	}
	if ((strcmp(attribute, "Saturation") == 0)
	    || (strcmp(attribute, "Hue") == 0)
	    || (strcmp(attribute, "Brightness") == 0)
	    || (strcmp(attribute, "Contrast") == 0))
	    return win32_SetRegValue(videoname, attribute, value, 0);
	if (strcmp(attribute, "maxauto") == 0)
	{
	    RegWriteInt("win32", "maxauto", value);
	    return 0;
	}
	if (info.FindAttribute(attribute))
	    return win32_SetRegValue(softname, attribute, value, 0);
        break;
/*    case RIFFINFO_WMV3:
	if (strcmp(attribute, "postprocessing") == 0
	    || strcmp(attribute, "Quality") == 0)
	{
	    //AVM_WRITE("Win32 plugin", "SETQUALITY %d\n", value);
	    return win32_SetRegValue((info.kind == CodecInfo::Win32)
				     ? crapname : softname,
				     "Current Post Process Mode",
				     value, 0);
	}*/
    case fccIV31:
    case fccIV32:
    case fccIV41:
    case fccIV50:
	if (strcmp(attribute, "QuickCompress") == 0
            || strcmp(attribute, "Transparency") == 0
            || strcmp(attribute, "Scalability") == 0
            || strcmp(attribute, "Saturation") == 0
	    || strcmp(attribute, "Brightness") == 0
	    || strcmp(attribute, "Contrast") == 0)
	    return win32_SetRegValue((info.fourcc == fccIV50)
				     ? indeo5name : indeo4name,
				     attribute, value, 0);
	break;
    case fccVP30:
    case fccVP31:
	if (strcmp(attribute, "strPostProcessingLevel") == 0
	    || strcmp(attribute, "strSettings") == 0)
	    return win32_SetRegValue(vp31name, attribute, value, 0);
        break;
    case fccXVID:
	return win32_SetRegValue(xvidname, attribute, value, 0);
    case fccMJPG:
	if (strcmp(info.dll, "m3jpeg32.dll") == 0)
	{
	    if (strcmp(attribute, "Mode") == 0)
	    {
		char s[256];
		sprintf(s, "%d", value);
		_WritePrivateProfileStringA("Compress", attribute, s, "M3JPEG.INI");
		return 0;
	    }
	}
	break;
    }

    AVM_WRITE("Win32 plugin", "SetAttrInt(): unknown attribute '%s' for codec %s\n",
	   attribute, (const char*) info.dll);
    return -1;
}

static int win32_SetAttrString(const CodecInfo& info, const char* attribute, const char* value)
{
    if (!attribute)
	return -1;

    switch (info.fourcc)
    {
    case fccMJPG:
	if (strcmp(info.dll, "m3jpeg32.dll") == 0)
	{
	    if (strcmp(attribute, "UserName") == 0
		|| strcmp(attribute, "LicenseKey") == 0)
	    {
		_WritePrivateProfileStringA("Register", attribute, value, "M3JPEG.INI");
		return 0;
	    }
	}
	break;
    }

    return -1;
}

static int win32_GetAttrString(const CodecInfo& info, const char* attribute, char* value, int size)
{
    if (!attribute)
	return -1;

    switch (info.fourcc)
    {
    case fccMJPG:
	if (strcmp(info.dll, "m3jpeg32.dll") == 0)
	{
	    if (strcmp(attribute, "UserName") == 0
		|| strcmp(attribute, "LicenseKey") == 0)
		return _GetPrivateProfileStringA("Register", attribute, "", value, size, "M3JPEG.INI");
	}
	break;
    }

    return -1;
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_win32;

avm::codec_plugin_t avm_codec_plugin_win32 =
{
    PLUGIN_API_VERSION,

    0,
    0, 0,
    avm::win32_GetAttrInt,
    avm::win32_SetAttrInt,
    0, 0,

    avm::win32_FillPlugins,
    avm::win32_CreateAudioDecoder,
    0,
    avm::win32_CreateVideoDecoder,
    avm::win32_CreateVideoEncoder,
};
