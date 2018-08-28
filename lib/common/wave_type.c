#include "utils.h"
#include "formats.h"
#include <string.h>
#include <stdio.h>

const char* avm_wave_format_name(short fmt) 
{
    switch ((unsigned short)fmt)
    {
    case WAVE_FORMAT_PCM: return "PCM";
    case WAVE_FORMAT_ADPCM: return "MS ADPCM";
    case WAVE_FORMAT_VSELP: return "Ima-0ADPCM VSELP";
    case WAVE_FORMAT_ALAW: return "A-Law";
    case WAVE_FORMAT_MULAW: return "u-Law";
    case WAVE_FORMAT_DVI_ADPCM: return "DVI ADPCM";
    case WAVE_FORMAT_GSM610:  return "MS GSM 6.10";
    case WAVE_FORMAT_MSNAUDIO: return "MSN Audio";
    case WAVE_FORMAT_MPEG: return "MPEG Layer-1/2";
    case WAVE_FORMAT_MPEGLAYER3: return "MPEG Layer-3";
    case WAVE_FORMAT_VOXWARE_BYTE_ALIGNED:
    case WAVE_FORMAT_VOXWARE_AC8:
    case WAVE_FORMAT_VOXWARE_AC10:
    case WAVE_FORMAT_VOXWARE_AC16:
    case WAVE_FORMAT_VOXWARE_RT24:
    case WAVE_FORMAT_VOXWARE_RT29HW:
    case WAVE_FORMAT_VOXWARE_VR12:
    case WAVE_FORMAT_VOXWARE_VR18:
    case WAVE_FORMAT_VOXWARE_RT29: return "Voxware Metasound";
    case WAVE_FORMAT_SIPROLAB_ACELP4800:
    case WAVE_FORMAT_SIPROLAB_ACELP8V3:
    case WAVE_FORMAT_SIPROLAB_G729:
    case WAVE_FORMAT_SIPROLAB_G729A:
    case WAVE_FORMAT_SIPROLAB_KELVIN:
    case WAVE_FORMAT_SIPROLAB_ACEPLNET: return "ACELP.net";
    case WAVE_FORMAT_MSAUDIO1: return "WMA v1";
    case 0x0161: return "WMA v2";
    case 0x0162: return "WMA v3";
    case 0x0401: return "Intel Music Coder";
    case 0x181c: return "VoxWare RT24 Speech codec";
    case 0x1fc4: return "ALF2";
    case WAVE_FORMAT_DVM: return "AC3 DVM";

    case WAVE_FORMAT_EXTENSIBLE: return "Extended Wave header";
    default:
	return "unknown?";
    }
}

char* avm_wave_format(char* buf, uint_t s, const WAVEFORMATEX* wf) 
{
    if (buf)
    {
	char b[200];
	sprintf(b, "fmt=0x%02x %uch %uHz %2ub %6uB/s balign=%u cbsz=%u",
		wf->wFormatTag, wf->nChannels, wf->nSamplesPerSec,
                wf->wBitsPerSample, wf->nAvgBytesPerSec, wf->nBlockAlign,
		wf->cbSize);
	memcpy(buf, b, s);
	buf[s - 1] = 0;
    }
    return buf;
}
