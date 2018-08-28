#include "SdlAudioRenderer.h"

#include "avifile.h"
#include "avm_cpuinfo.h"
#include "avm_output.h"

#include <SDL.h>
#include <SDL_audio.h>

#include <string.h>
#include <stdlib.h>

#ifndef SDL_VERSIONNUM
#define SDL_VERSIONNUM(X, Y, Z)  (X)*1000 + (Y)*100 + (Z)
#endif

#define _SDL_VER SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL)

AVM_BEGIN_NAMESPACE;

class SdlAudioMix : public IAudioMix
{
    int m_iVolume;
public:
    SdlAudioMix() { SetVolume(); }
    virtual int Mix(void* data, const void* src, uint_t n) const
    {
	// SDL should use const pointer for src
	if (m_iVolume != SDL_MIX_MAXVOLUME)
	    SDL_MixAudio((Uint8*) data, (Uint8*) src, n, m_iVolume);
	else
	    memcpy(data, src, n);

	return n;
    }
    void SetVolume(int volume = SDL_MIX_MAXVOLUME) { m_iVolume = volume; }
};

int SdlAudioRenderer::Init()
{
    for (;;)
    {
	sdl_systems = 0;
#if _SDL_VER < 1103
	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
#else
	Uint32 subsystem_init = SDL_WasInit(SDL_INIT_EVERYTHING);

	if (subsystem_init == 0)
	{
	    SDL_Init(SDL_INIT_NOPARACHUTE);
	    atexit(SDL_Quit);
	}
	if (!(subsystem_init & SDL_INIT_AUDIO))
	{
	    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	    {
		AVM_WRITE("SDL audio renderer", "failed to init SDL_AUDIO!\n");
                break;
	    }
            sdl_systems |= SDL_INIT_AUDIO;
	}
#endif
	if (m_Owf.wFormatTag != 0x01)
	{
	    AVM_WRITE("SDL audio renderer", "unsupported audio format 0x%x!\n", m_Owf.wFormatTag);
            break;
	}

	SDL_AudioSpec spec;
	SDL_AudioSpec m_Spec;
        memset(&spec, 0, sizeof(spec)); // keep quiet memory validator
	spec.freq = (m_uiUseFreq) ? m_uiUseFreq : m_Owf.nSamplesPerSec;
	spec.format = (m_Owf.wBitsPerSample == 16) ? AUDIO_S16 : AUDIO_U8;
	spec.channels = m_Owf.nChannels;
	spec.samples = 2048; // 1024 causes drops on busy system
	// if (m_Owf.nChannels == 1) spec.samples *= 2;
	// shorter buffer generates more interrupts
	// and would give us more precise timing -
	// on the other hand it's more sensitive to the system load
        // for slowly moving sound - shorter buffer is better
	spec.callback = fillAudio;
	spec.userdata = this;

	//printf("SDL freq:%d fmt:0x%x  ch:%d s:%d\n", spec.freq, spec.format, spec.channels, spec.samples);
	// to simulate broken audio card
	// spec.freq += -1000;
	if (SDL_OpenAudio(&spec, &m_Spec) < 0)
	{
	    SDL_CloseAudio();
	    AVM_WRITE("SDL audio renderer", "%s!\n", SDL_GetError());
            break;
	}

	AVM_WRITE("SDL audio renderer", 1, "error: %s\n", SDL_GetError());
	m_dSpecTime = m_Spec.size/(double) m_pQueue->GetBytesPerSec();
        m_pAudiomix = new SdlAudioMix();
	AVM_WRITE("SDL audio renderer", 0, "buffer size: %d  %dHz\n", m_Spec.size, m_Spec.freq);
        return 0;
    }

    return -1;
}

SdlAudioRenderer::~SdlAudioRenderer()
{
    AVM_WRITE("SDL audio renderer", 2, "destroy()\n");
    m_pQueue->Lock();
    m_bQuit = true;
    m_bInitialized = false;
    m_pQueue->Unlock();
    m_pQueue->Broadcast();
    if (m_pAudiomix)
    {
	delete m_pAudiomix;
	SDL_CloseAudio();
    }
#if _SDL_VER > 1102
    //we can't call SDL_Quit() for older SDL - causes crash
    if (sdl_systems & SDL_INIT_AUDIO)
	SDL_QuitSubSystem(sdl_systems);
#endif
    AVM_WRITE("SDL audio renderer", 2, "destroy() successful\n");
}

int SdlAudioRenderer::SetVolume(int volume)
{
    int hr = IAudioRenderer::SetVolume(volume);
    if (hr == 0)
	m_pAudiomix->SetVolume(volume * SDL_MIX_MAXVOLUME / VOL_MAX);
    return hr;
}

void SdlAudioRenderer::fillAudio(void* userdata, unsigned char* stream, int len)
{
    //AVM_WRITE("SDL audio renderer", 3, "fillAudio()\n");
    SdlAudioRenderer& a = *(SdlAudioRenderer*)userdata;
    a.m_pQueue->Lock();
    while (!a.m_bQuit && a.m_bInitialized)
    {
	if (a.m_pQueue->GetSize() < unsigned(len))
	{
	    //printf("EOF: %d\n", a.m_pAudiostream->Eof());
	    memset(stream, 0, len);
	    if (a.m_pAudiostream->Eof())
	    {
                // play the rest
                len = a.m_pQueue->GetSize();
		if (len == 0)
		    break;
	    }
	    else
	    {
                // wait for buffer being filled
		a.m_pQueue->Wait(0.01);
		continue;
	    }
	}

#if 0
	int64_t ct = longcount();
	static int64_t las = ct;
	AVM_WRITE("SDL audio renderer", "diff %f\n", to_float(ct, las)); las = ct;
#endif
	a.m_pQueue->Read(stream, len, a.m_pAudiomix);
        a.updateTimer();
#if 0
	SDL_AudioSpec spec;
	spec.freq = (a.m_uiUseFreq) ? a.m_uiUseFreq : a.m_Owf.nSamplesPerSec;
	spec.format = (a.m_Owf.wBitsPerSample == 16) ? AUDIO_S16 : AUDIO_U8;
	spec.channels = a.m_Owf.nChannels;
	AVM_WRITE("SDL audio renderer", "OLDAUDIO  freq:%d  format:%d  channels:%d\n",
		  a.m_Spec.freq, a.m_Spec.format, a.m_Spec.channels);

	AVM_WRITE("SDL audio renderer", "NEWAUDIO  freq:%d  format:%d  channels:%d\n",
		  spec.freq, spec.format, spec.channels);
#endif
	break;
    }
    a.m_pQueue->Unlock();
}

void SdlAudioRenderer::pause(int v)
{
    SDL_PauseAudio(v);
}

#undef _SDL_VER

AVM_END_NAMESPACE;
