#include "AudioIntResampler.h"

AVM_BEGIN_NAMESPACE;

template <class T>
uint_t AudioIntResamplerMono<T>::getBitsPerSample() const
{
    return sizeof(T) * 8;
}

template <class T>
uint_t AudioIntResamplerStereo<T>::getBitsPerSample() const
{
    return sizeof(T) * 8;
}

template <class T>
void AudioIntResamplerMono<T>::resample(void* out, const void* in, uint_t dest_len, uint_t src_len)
{
    const T* src = (const T*) in;
    T* dest = (T*) out;

    double rate = src_len / (double) dest_len;
    double progress = 0;

    for (T* d = dest; d < &dest[dest_len * 2];)
    {
	*d++ = src[int(progress)];
	progress += rate;
    }
}

template <class T>
void AudioIntResamplerStereo<T>::resample(void* out, const void* in, uint_t dest_len, uint_t src_len)
{
    const T* src = (const T*) in;
    T* dest = (T*) out;

    double rate = src_len / (double) dest_len;
    double progress = 0;

    for (T* d = dest; d < &dest[dest_len * 2];)
    {
	*d++ = src[int(progress) * 2];
	*d++ = src[int(progress) * 2 + 1];
	progress += rate;
    }
}

// explicitly create implementation for these types
// we have only one piece of the code for all these type
template class AudioIntResamplerStereo<char>;
template class AudioIntResamplerStereo<short>;
template class AudioIntResamplerStereo<int>;

template class AudioIntResamplerMono<char>;
template class AudioIntResamplerMono<short>;
template class AudioIntResamplerMono<int>;


/*
static int multiply(char* dest, const char* src, uint_t src_size, int rate, int samplesize);

if this would be useful or there will be free time
reimplement this here correctly

int AudioQueue::multiply(char* dest, const char* src, uint_t src_size, int rate, int samplesize)
{
    Debug cout << "AudioQueue: multiply" << endl;
    unsigned  i;
    switch(rate*16+samplesize)
    {
    case 0x21:
	    for(i=0; i<src_size; i++)
	    {
		dest[0]=src[0];
	        dest[1]=src[0];
		dest+=2;
		src++;
    	    }
	    break;
    case 0x22:
	    for(i=0; i<src_size/2; i++)
	    {
		*(short*)dest=*(const short*)src;
		dest+=2;
		*(short*)dest=*(const short*)src;
		dest+=2;
		src+=2;
    	    }
	    break;
    case 0x24:
	    for(i=0; i<src_size/4; i++)
	    {
		*(int*)dest=*(const int*)src;
		dest+=4;
		*(int*)dest=*(const int*)src;
		dest+=4;
		src+=4;
    	    }
	    break;
    case 0x41:
	    for(i=0; i<src_size; i++)
	    {
		dest[0]=src[0];
		dest[1]=src[0];
		dest[2]=src[0];
		dest[3]=src[0];
		dest+=4;
		src++;
	    }
	    break;
    case 0x42:
	    for(i=0; i<src_size/2; i++)
	    {
		*(short*)dest=*(const short*)src;
		dest+=2;
		*(short*)dest=*(const short*)src;
		dest+=2;
		*(short*)dest=*(const short*)src;
		dest+=2;
		*(short*)dest=*(const short*)src;
		dest+=2;
		src+=2;
	    }
	    break;
    case 0x44:
	    for(i=0; i<src_size/4; i++)
	    {
		*(int*)dest=*(const int*)src;
		dest+=4;
		*(int*)dest=*(const int*)src;
		dest+=4;
		*(int*)dest=*(const int*)src;
		dest+=4;
		*(int*)dest=*(const int*)src;
		dest+=4;
		src+=4;
	    }
	    break;
    }
    return src_size*rate;
}
*/

AVM_END_NAMESPACE;
