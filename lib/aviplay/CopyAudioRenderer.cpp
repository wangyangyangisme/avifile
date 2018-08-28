/*********************************************************

	CopyAudioRenderer implementation

*********************************************************/

#include "aviplay.h" // AUDIOFUNC
#include "CopyAudioRenderer.h"
#include "AudioQueue.h"
#include "avifile.h"
#include "utils.h"

AVM_BEGIN_NAMESPACE;

int CopyAudioMix::Mix(void* data, const void* src, uint_t n) const
{
    return m_pAf((void*) src, n, m_pArg);
}

CopyAudioRenderer::CopyAudioRenderer(IReadStream* stream, WAVEFORMATEX& Owf, AUDIOFUNC func, void* arg)
    : IAudioRenderer(stream, Owf)
{
    m_pAudioMix = new CopyAudioMix(func, arg);
    m_pQueue->DisableCleaner();
}

int CopyAudioRenderer::Extract()
{
    IAudioRenderer::Extract();

    uint_t size = m_pQueue->GetSize();

    m_dAudioRealpos = m_pAudiostream->GetTime()
	- m_pQueue->GetBufferTime() - m_fAsync;

    if (m_bInitialized && !m_bPaused && size > 0)
	m_pQueue->Read(0, size, m_pAudioMix);

    return 0;
}

AVM_END_NAMESPACE;
