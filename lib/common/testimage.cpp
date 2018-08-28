#include "image.h"
#include "cpuinfo.h"
#include "mmx.h"
#include "avm_fourcc.h"
#include "avm_output.h"
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <math.h>


#include "cpuinfo.h"
#include "utils.h"
#include "aviplay.h"

#define USEMMX
//#define USE8

void mmx_copy(void* to, const void* from, uint_t size)
{
    //memcpy(to, from, size);
    //return;
    uint_t s = size & 0x3f;
    if (s)
    {
        int dummy;
	__asm__ __volatile__
	    (
	     "rep; movsb"
	     :"=&D"(to), "=&S"(from), "=&c"(dummy)
	     :"0" (to), "1" (from),"2" (s)
	     : "memory"
	    );
        if (size < 0x40)
	    return;
        size &= 0x3f;
    }

#define MOVNTQ "movntq"
//#define MOVNTQ "movq"
    //printf("FROM %p  %p \n", from, to);
    __asm__ __volatile__ (".balign 8");
    do {
	__asm__ __volatile__
	    (
#ifdef USE8
#if 1
	     "movq   (%0, %2), %%mm0\n\t"
	     "movq %%mm0,   (%1, %2)\n\t"
	     "movq  8(%0, %2), %%mm1\n\t"
	     "movq 16(%0, %2), %%mm2\n\t"
	     "movq 24(%0, %2), %%mm3\n\t"
	     "movq 32(%0, %2), %%mm4\n\t"
	     "movq 40(%0, %2), %%mm5\n\t"
	     "movq 48(%0, %2), %%mm6\n\t"
	     "movq 56(%0, %2), %%mm7\n\t"

	     "movq %%mm1,  8(%1, %2)\n\t"
	     "movq %%mm2, 16(%1, %2)\n\t"
	     "movq %%mm3, 24(%1, %2)\n\t"
	     "movq %%mm4, 32(%1, %2)\n\t"
	     "movq %%mm5, 40(%1, %2)\n\t"
	     "movq %%mm6, 48(%1, %2)\n\t"
	     "movq %%mm7, 56(%1, %2)\n\t"
#else
	     "movq   (%0, %2), %%mm0\n\t"
	     "movq 32(%0, %2), %%mm4\n\t"
	     //"movq %%mm0,   (%1, %2)\n\t"
	     "movq  8(%0, %2), %%mm1\n\t"
	     "movq 40(%0, %2), %%mm5\n\t"
	     "movq 16(%0, %2), %%mm2\n\t"
	     "movq 24(%0, %2), %%mm3\n\t"
	     "movq 48(%0, %2), %%mm6\n\t"
	     "movq 56(%0, %2), %%mm7\n\t"

	     "prefetchnta 64(%0, %2)\n\t"
	     "prefetchnta 96(%0, %2)\n\t"
	     "prefetchnta 320(%0, %2)\n\t"

	     MOVNTQ" %%mm0,   (%1, %2)\n\t"
	     MOVNTQ" %%mm1,  8(%1, %2)\n\t"
	     MOVNTQ" %%mm2, 16(%1, %2)\n\t"
	     MOVNTQ" %%mm3, 24(%1, %2)\n\t"
	     MOVNTQ" %%mm4, 32(%1, %2)\n\t"
	     MOVNTQ" %%mm5, 40(%1, %2)\n\t"
	     MOVNTQ" %%mm6, 48(%1, %2)\n\t"
	     MOVNTQ" %%mm7, 56(%1, %2)\n\t"
#endif
#else
	     "movq   (%0, %2), %%mm0\n\t"
	     "movq %%mm0,   (%1, %2)\n\t"
	     "movq  8(%0, %2), %%mm1\n\t"
	     "movq 16(%0, %2), %%mm2\n\t"
	     "movq 24(%0, %2), %%mm3\n\t"
	     //"movl 32(%0, %2), %%eax\n\t"
	     "movq %%mm1,  8(%1, %2)\n\t"
	     "movq %%mm2, 16(%1, %2)\n\t"
	     "movq %%mm3, 24(%1, %2)\n\t"
#endif
	     :
	     :"r"(from), "r"(to), "r"(s)
	     :"eax", "memory"
	    );
#ifdef USE8
	s += 64;
	//(const char*)from += 64;
        //(char*)to += 64;
#else
	s += 32;
#endif
    } while (s < size);

    __asm__ __volatile__ ("emms");
}

#define MOVQ_WONE(regd) \
    __asm __volatile ( \
       "pcmpeqd %%" #regd ", %%" #regd " ;" \
       "psrlw $15, %%" #regd " ;")

#define MOVQ_WTWO(regd) \
    __asm __volatile ( \
       "pcmpeqd %%" #regd ", %%" #regd " \n\t" \
       "psrlw $14, %%" #regd " \n\t")

int main(int argc, char* argv[])
{
    GetAvifileVersion();
#if 0
    int MB = 1024 * 1024;
    char* a = (char*) memalign(64, 8 * MB);
    for (int i = 0; i < 8 * MB; i++) a[i] = (int) (i);
    int x = 10, y = 0;

//    MOVQ_WONE(mm7);
    __asm __volatile
	(
	 " pcmpeqd %%mm7, %%mm7 ;"
	 " psrlw $15, %%mm7 ;"
	 " psllw $1, %%mm7 ;"
	 " movd %%mm7, %%eax ;"
	 " movl %%eax, %0 ;"
	 :"=r"(y)
	 :"r"(x)
	 :"%eax");

    printf("Regset 0x%x\n", y);
    return 0;
#endif
#if 0
    int ite;
    for (int j = 1624/2; j < 2024/2; j += 32)
    {
        MB = 1024 * j;
	int64_t m1 = longcount();
	for (ite = 0; ite < 20; ite++)
#ifdef USEMMX
	    mmx_copy(a + MB , a, MB & ~0x3f);
#else
            memcpy(a +  MB, a, MB & ~0x3f);
#endif
	float tt = to_float(longcount(), m1);
	AVM_WRITE("MMX_COPY", "TIME %f    %fMB/s  %d  %dB\n", tt, (ite * MB / 1024 / 1024) / tt,
		  memcmp(a, a + 2 * MB, MB), MB);
    }
    for (int i = 0; i < 4000000; i++)
	a[i] = (int) (a + i);

    return 0;
#endif

    BITMAPINFOHEADER bh;
    bh.biSize = sizeof(BITMAPINFOHEADER);
    bh.biWidth = 640;
    bh.biHeight = 480;
    bh.biCompression = 0;

    BitmapInfo bii(bh);
    BitmapInfo bio(bh);
    //bii.SetBits(24);
    bii.SetSpace(IMG_FMT_YUY2);
    //bio.SetBits(15);
    bio.SetSpace(IMG_FMT_YV12);
    CImage* cii = new CImage(&bii);
    CImage* cio = new CImage(&bio);

    int64_t t1 = longcount();
    const int iter = 100;//0;
    for (int i = 0; i < iter; i++)
	cio->Convert(cii);
    int64_t t2 = longcount();
    float tm = to_float(t2, t1);

    AVM_WRITE("CImage", "TIME %f    %f\n", tm, tm / iter);

    return 0;
}
