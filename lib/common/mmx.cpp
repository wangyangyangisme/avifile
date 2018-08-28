/********************************************************

Miscellaneous MMX-accelerated routines
Copyright 2000 Eugene Kuznetsov (divx@euro.ru)

*********************************************************/

#include "avm_cpuinfo.h"
#include "mmx.h"
#include <stdio.h>
#include <stdlib.h>

#if !defined WIN32 && defined ARCH_X86

static inline void emms(void)
{
    __asm __volatile ("emms;": : :"memory");
}

#define emms_c() \
{ \
    emms(); \
}

void mmx_copy(void* to, const void* from, uint_t size)
{
    uint_t sx = size;
    uint_t s = size & 0x3f;
    if (s)
    {
        int dummy;
	asm volatile
	    (
	     "rep; movsb"
	     :"=&D"(to), "=&S"(from), "=&c"(dummy)
	     :"0"(to), "1"(from), "2"(s)
	     : "memory"
	    );
    }

    while (s < sx)
    {
	asm volatile
	    (
#if 0
	     ".balign 16            \n\t"
	     "1:                    \n\t"
	     "movq   (%0, %2), %%mm0\n\t"
	     "movq  8(%0, %2), %%mm1\n\t"
	     "movq 16(%0, %2), %%mm2\n\t"
	     "movq 24(%0, %2), %%mm3\n\t"
	     "movq 32(%0, %2), %%mm4\n\t"
	     "movq 40(%0, %2), %%mm5\n\t"
	     "movq 48(%0, %2), %%mm6\n\t"
	     "movq 56(%0, %2), %%mm7\n\t"
	     "movq %%mm0,   (%1, %2)\n\t"
	     "movq %%mm1,  8(%1, %2)\n\t"
	     "movq %%mm2, 16(%1, %2)\n\t"
	     "movq %%mm3, 24(%1, %2)\n\t"
	     "movq %%mm4, 32(%1, %2)\n\t"
	     "movq %%mm5, 40(%1, %2)\n\t"
	     "movq %%mm6, 48(%1, %2)\n\t"
	     "movq %%mm7, 56(%1, %2)\n\t"
#else
	     ".balign 16            \n\t"
	     "1:                    \n\t"
	     "movq   (%0, %2), %%mm0\n\t"
	     "movq  8(%0, %2), %%mm1\n\t"
	     "movq %%mm0,   (%1, %2)\n\t"
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
#endif
	     :
	     :"r"(from), "r"(to), "r"(s)
	     :"memory"
	    );
        s += 64;
    }
    emms_c();
}


static void zoom_16_bpp(uint16_t* dest, const uint16_t* src, int dst_w, int dst_h, int src_w, int src_h, int xdim)
{
    int x_min = (src_w%dst_w);
    int y_min = src_h%dst_h;
    int x_accum = 0;
    int y_accum = 0;
    const uint16_t *src2=src;
    int dest_delta = (xdim!=0 ? xdim-dst_w : 0);

    int x_maj = src_w/dst_w*2;
    int y_maj = (src_h/dst_h)*src_w;
    if ((x_maj>0) || (y_maj>0))
    {
	for(int i=0; i<dst_h; i++)
	{
	    int j = dst_w;
	    //printf("%d  %p  %p  j:%d\n", y_accum, dest, src2, j);
	    // for(int j=0; j<dst_w; j++)
	    asm volatile
		("pushl %%ebx		\n\t"
		 "1:			\n\t"
		 //   *(dest+j)=*src;
		 "movw   (%%eax),%%bx	\n\t"
		 "movw   %%bx, (%%ecx)	\n\t"
		 //   dest++;
		 "addl   $2, %%ecx	\n\t"
		 //   src+=x_maj;
		 "addl   %9, %%eax	\n\t"
		 //   x_accum-=x_min;
		 "subl   %6, %%edx	\n\t"
		 //   if(x_accum<=0) {
		 "jnc    x2		\n\t"
		 //      xaccum+=dst_w;
		 "addl   %7, %%edx	\n\t"
		 //         src++;
		 "addl   $2, %%eax	\n\t"
		 "x2:			\n\t"
		 "decl   %8		\n\t"
		 "jnz    1b		\n\t"
		 "popl   %%ebx		\n\t"
		 : "=a"(src), "=c"(dest), "=d"(x_accum)           // output
		 : "a"(src),"c"(dest), "d"(x_accum), "r"(x_min), "r"(dst_w), "r"(j), "g"(x_maj)
		 :"memory"
		);

	    dest+=dest_delta;
	    src2+=y_maj;
	    y_accum+=y_min;
	    if(y_accum>=dst_h)
	    {
		y_accum-=dst_h;
		src2+=src_w;
	    }

	    src=src2;
	}
    }
    else
	//     cout << "FIXME error - Tried to use ASM code which is not shlib friendly" << endl;

	// has to be fixed - causes reference .rel.text
	// objdump --headers --private-headers -T libaviplay.so | less
	// if this output doesn't contain .rel.text than its OK!
#if 0
//      BUGGY
    {
	int dest2=0;
	int i=0;
	__asm__ __volatile__
	    (
	     "pushal                         \n\t"
	     "subl $16, %%esp		     \n\t"
	     "movl  %5, %%ebx                \n\t"   // 8(%%esp) = dest_h
	     "movl  %%ebx, 8(%%esp)          \n\t"
	     "movl  %4, %%ebx                \n\t"   // 12(%%esp) = dest_w
	     "movl  %%ebx, 12(%%esp)         \n\t"
	     "movl  %5, %%ebx                \n\t"   // i = src_h

	     "movl  %%ebx, (%%esp)           \n\t"

	     "for_i:                         \n\t"
	     "movl  %0, 4(%%esp)             \n\t"   // 4(%%esp) = dest

	     "movl  12(%%esp), %%edx         \n\t"

	     "movl  %4, %%ecx                \n\t"   // j = src_w
	     "for_j:                         \n\t"

	     "movw  (%1), %%bx               \n\t"

	     "x_dest_copy_again:             \n\t"
	     "movw   %%bx, (%0)              \n\t"   // *dest = *src
	     "addl   $2, %0                  \n\t"   // dest++;
	     "subl   %6, %%edx               \n\t"   // 12(%%esp)-=x_min
	     "jnc    x_dest_copy_again       \n\t"   // while (12(%%esp) > 0)

	     "no_xdestcopy:                  \n\t"
	     "addl   %2, %%edx               \n\t"   // 12(%%esp) += dest_w
	     "addl   $2, %1                  \n\t"   // src++;
	     "decl   %%ecx                   \n\t"   // j--;
	     "jnz    for_j                   \n\t"   // while (j>0) Next

	     "y:                             \n\t"
	     "movl   %%edx, 12(%%esp)        \n\t"

	     "addl   %8, %0                  \n\t"   // dest += dest_delta

	     "movl   8(%%esp), %%edx         \n\t"
	     "push   %1                      \n\t"   // safe src

	     "y_dest_copy_again:             \n\t"
	     "subl   %7, %%edx               \n\t"   // 8(%%esp) -= y_min
	     "jc     y_no_cpy_again          \n\t"   // while (8(%%esp)>0)
	     "movl   8(%%esp),%%esi          \n\t"
	     "movl   %2, %%ecx               \n\t"
	     "shrl   $1,%%ecx                \n\t"
	     "cld                            \n\t"
	     "rep; movsl                     \n\t"
	     "addl   %8, %0                  \n\t"   // dest += dest_delta

	     "jmp    y_dest_copy_again	\n\t"

	     "y_no_cpy_again:		\n\t"
	     "addl   %3, %%edx		\n\t"   // 8(%%esp) += dest_h
	     "movl   %%edx, 12(%%esp)	\n\t"

	     "pop    %1			\n\t"   // get src

	     "decl   (%%esp)		\n\t"
	     "jnz    for_i		\n\t"
	     "addl   $16, %%esp		\n\t"

	     "popal			\n\t"
	     :
	     : "D"(dest), "S"(src), "r"(dst_w), "r"(dst_h), "r"(src_w), "r"(src_h), "a"(x_min), "r"(y_min), "r"(dest_delta)
	     :"memory"
	    );
    }
#else
    {
	// slow dumb implementation which doesn't work
	uint16_t* pdest = dest;
	for (int i = 0; i < src_w && i < dst_h; i++)
	{
	    for (int j = 0; j < src_h; j++)
		pdest[j] = *src++;
	    pdest += dst_w;
	}
    }
#endif
}


static void zoom_24_bpp(int* dest, const int* src, int dst_w, int dst_h, int src_w, int src_h, int xdim)
{
    int x_maj = src_w/dst_w;
    int x_min = src_w%dst_w;
    int y_maj = (src_h/dst_h)*src_w;
    int y_min = src_h%dst_h;
    int x_accum = 0;
    int y_accum = 0;
    const int *src2=src;
    int dest_delta = (xdim!=0 ? 3*(xdim-dst_w) : 0);

    //    cout << "(" << dst_w << "," << dst_h << "," << src_w << "," << src_h << ")" << endl;
    for (int i=0; i<dst_h; i++)
    {

	for(int j=0; j<dst_w; j++)
	{
	    *dest=*src;
	    dest=(int*)((char*)dest+3);

	    src=(const int*)((const char*)src+x_maj*3);
	    x_accum+=x_min;
	    if(x_accum>=dst_w) {
		x_accum-=dst_w;
		src=(const int*)((const char*)src+3);
	    }
	}

	dest = (int*) ((char*) dest + dest_delta);

	src2=(const int*)((const char*)src2+y_maj*3);
	y_accum+=y_min;
	if(y_accum>=dst_h)
	{
	    y_accum-=dst_h;
	    src2=(const int*)((const char*)src2+3*src_w);
	}
	src=src2;
    }
}

static void zoom_32_bpp(int* dest, const int* src, int dst_w, int dst_h, int src_w, int src_h, int xdim)
{
    int x_maj = src_w/dst_w;
    int x_min = src_w%dst_w;
    int y_maj = (src_h/dst_h)*src_w;
    int y_min = src_h%dst_h;
    int x_accum = 0;
    int y_accum = 0;
    const int *src2=src;
    int dest_delta = (xdim!=0 ? xdim-dst_w : 0);

    //    cout << "(" << dst_w << "," << dst_h << "," << src_w << "," << src_h << ")" << endl;
    for(int i=0; i<dst_h; i++)
    {
	for(int j=0; j<dst_w; j++)
	{
	    *dest=*src;
	    dest++;

	    src+=x_maj;
	    x_accum+=x_min;
	    if(x_accum>=dst_w) {
		x_accum-=dst_w;
		src++;
	    }
	}

	dest+=dest_delta;
	src2+=y_maj;
	y_accum+=y_min;
	if(y_accum>=dst_h)
	{
	    y_accum-=dst_h;
	    src2+=src_w;
	}
	src=src2;
    }
}

void zoom(uint16_t* dest, const uint16_t* src, int dst_w, int dst_h, int src_w, int src_h, int bpp, int xdim)
{
    switch(bpp)
    {
    case 15:
    case 16:
	return zoom_16_bpp(dest,src,dst_w,dst_h,src_w,src_h,xdim);
    case 24:
	return zoom_24_bpp((int*)dest,(const int*)src,dst_w,dst_h,src_w,src_h,xdim);
    case 32:
	return zoom_32_bpp((int*)dest,(const int*)src,dst_w,dst_h,src_w,src_h,xdim);
    }
}

#endif // WIN32


static void zoom_2_16_nommx(uint16_t* dest, const uint16_t* src, int w, int h)
{
    for(int i=0; i<h; i+=2)
    {
	for(int j=0; j<w; j+=2)
	{
	    *dest=*src;
	    dest++;
	    src+=2;
	}
	src+=w;
    }
}

static void zoom_2_16_to565_nommx(uint16_t *dest, const uint16_t *src, int w, int h)
{
    //    dest+=w/2*(h/2-1);
    for(int i=0; i<h/2; i++)
    {
	for(int j=0; j<w/2; j++)
	{
	    uint16_t q=*src;
	    q+=(q&0xFFE0);
	    *dest++=q;
	    src+=2;
	}
	src+=w;
	//	dest-=w;
    }
}

static void v555to565_nommx(uint16_t* dest, const uint16_t* src, int w, int h)
{
    // one line - convertor is in image.cpp anyway
    // this code is not used
    uint16_t* s = (uint16_t*) src;
    uint16_t* d = (uint16_t*) dest;
    while (w > 1)
    {
	w -= 2;
	unsigned q = *((uint32_t *)&s[w]);
	*((uint32_t *)&d[w]) = (q & 0x7FFF7FFF) + (q & 0x7FE07FE0);
    }
    if (w)
    {
	uint16_t q = s[0];
	q += (q & 0x7FE0);
	d[0] = q;
    }
}

#if !defined WIN32 && defined ARCH_X86
/*********************************************

WARNING

All MMX code assumes that dest scanline sizes
are multiple of 8 bytes.

*********************************************/
static void v555to565_mmx(uint16_t* dest, const uint16_t* src, int w, int h)
{
    static uint64_t line __attribute__ ((aligned(8))) = 0xFFE0FFE0FFE0FFE0LL;

    bool flip=(h<0);
    if(flip)
    {
	h=-h;
	src+=w*(h-1);
    }
    __asm__ __volatile__
	(
	 "movq %0, %%mm2		\n\t"
	 : : "m"(line)
	);

    //    dest+=w*(h-1);

    for(int i=0; i<h; i++)
    {
	__asm__ __volatile__
	    (
	     "1:			\n\t"
	     "movq  (%0),  %%mm0	\n\t"
	     "movq  %%mm0, %%mm1	\n\t"
	     "pand  %%mm2, %%mm1	\n\t"
	     "paddw %%mm1, %%mm0	\n\t"
	     "movq  %%mm0, (%1)		\n\t"
	     "addl     $8, %0		\n\t"
	     "addl     $8, %1		\n\t"
	     "cmpl     %0, %2		\n\t"
	     "ja    1b			\n\t"
	     : :"r"(src), "r"(dest), "r"(src + 2*w)
	     :"memory"
	    );
	if(flip)
	    src-=w;
	else
	    src+=w;
	dest+=w;
    }
    emms_c();
}

static void zoom_2_16_to565_mmx(uint16_t *dest, const uint16_t *src, int w, int h)
{
    static uint64_t line __attribute__ ((aligned(8))) = 0xFFE0FFE0FFE0FFE0LL;
    static uint64_t line2 __attribute__ ((aligned(8))) = 0x0000FFFF0000FFFFLL;

    __asm__ __volatile__
	(
	 "movq %0, %%mm2		\n\t"
	 "movq %1, %%mm3		\n\t"
	 : : "m"(line), "m"(line2)
	);

    //    dest+=w/2*(h/2-1);
    for(int i=0; i<h/2; i++)
    {
	__asm__ __volatile__
	    (
	     "1:			\n\t"
	     //Load 8 subsequent pixels into mm0 and mm1.
	     //Drop each second one by pand.
	     "movq   (%0), %%mm0       	\n\t"
	     "pand  %%mm3, %%mm0       	\n\t"
	     "movq  8(%0), %%mm1	\n\t"
	     "addl    $16, %%eax       	\n\t"
	     "pand  %%mm3, %%mm1       	\n\t"
	     //Pack 4 remaining pixels into mm0.
	     "packssdw %%mm1,%%mm0	\n\t"
	     //Convert 555 -> 565.
	     "movq  %%mm0, %%mm1	\n\t"
	     "pand  %%mm2, %%mm1	\n\t"
	     "paddw %%mm1, %%mm0	\n\t"
	     //Store the result.
	     "movq  %%mm0, (%1)		\n\t"

	     "addl     $8, %%ecx	\n\t"
	     "cmpl  %0, %2		\n\t"
	     "ja     1b			\n\t"

	     :
	     :"r"(src), "r"(dest), "r"(src + 2*w)
	     :"memory"
	    );
	src+=2*w;
	dest+=w/2;
    }
    emms_c();
}


static void zoom_2_16_mmx(uint16_t *dest, const uint16_t *src, int w, int h)
{
    static uint64_t line2 __attribute__ ((aligned(8))) = 0x0000FFFF0000FFFFLL;

    __asm__ __volatile__
	(
	 "movq %0, %%mm3		\n\t"
	 : : "m"(line2)
	);

    for(int i=0; i<h/2; i++)
    {
	__asm__ __volatile__
	    (
	     "1:			\n\t"
	     //Load 8 subsequent pixels into mm0 and mm1.
	     //Drop each second one by pand.
	     "movq    (%0), %%mm0	\n\t"
	     "pand   %%mm3, %%mm0	\n\t"
	     "movq   8(%0), %%mm1	\n\t"
	     "addl     $16, %%eax	\n\t"
	     "pand   %%mm3, %%mm1	\n\t"
	     //Pack 4 remaining pixels into mm0.
	     "packssdw %%mm1, %%mm0	\n\t"
	     //Store the result.
	     "movq     %%mm0, (%1)	\n\t"

	     "addl       $8, %1		\n\t"
	     "cmpl     %0, %2		\n\t"
	     "ja      1b       		\n\t"

	     : :"r"(src), "r"(dest), "r"(src + 2*w)
	     :"memory"
	    );
	src+=2*w;
	dest+=w/2;
    }
    emms_c();
}


void zoom_2_32_mmx(uint32_t *dest, const uint32_t *src, int w, int h)
{
    static uint64_t line2 __attribute__ ((aligned(8))) = 0x00000000FFFFFFFFLL;

    //    dest+=w/2*(h/2-1);
    for(int i=0; i<h/2; i++)
    {
	__asm__ __volatile__
	    (
	     "1:			\n\t"
	     //Load 4 subsequent pixels into mm0 and mm1.
	     "movq  (%0), %%mm0		\n\t"
	     "movq  8(%0), %%mm1	\n\t"
	     "addl  $16, %0		\n\t"
	     //Put 2 pixels into mm0.
	     //this should work, but I'm too lazy to check it.
	     "punpckhdq %%mm1, %%mm0	\n\t"
	     //Store the result.
	     "movq  %%mm0, (%1)		\n\t"

	     "addl  $8, %1		\n\t"
	     "cmpl  %0, %2		\n\t"
	     "ja    1b			\n\t"

	     : :"r"(src), "r"(dest), "r"(src + 4 * w)
	     :"memory"
	    );
	src+=2*w;
	dest+=w/2;
    }
    emms_c();
}

void zoom_2_32_nommx(uint32_t *dest, const uint32_t *src, int w, int h)
{
    for (int i=0; i<h/2; i++)
    {
	for (int j=0; j<w/2; j++)
	{
	    *dest=*src;
	    dest++;
	    src+=2;
	}
	src+=w;
    }
}

static void v555to565_stub(uint16_t* dest, const uint16_t* src, int w, int h)
{
    if (freq.HaveMMX())
	v555to565=v555to565_mmx;
    else
	v555to565=v555to565_nommx;

    v555to565(dest,src,w,h);
}

static void zoom_2_16_stub(uint16_t* dest, const uint16_t* src, int w, int h)
{
    if(freq.HaveMMX())
	zoom_2_16=zoom_2_16_mmx;
    else
	zoom_2_16=zoom_2_16_nommx;
    zoom_2_16(dest,src,w,h);
}

static void zoom_2_16_to565_stub(uint16_t *dest, const uint16_t *src, int w, int h)
{
    if(freq.HaveMMX())
	zoom_2_16_to565=zoom_2_16_to565_mmx;
    else
	zoom_2_16_to565=zoom_2_16_to565_nommx;
    zoom_2_16_to565(dest,src,w,h);
}

static void zoom_2_32_stub(uint32_t *dest, const uint32_t *src, int w, int h)
{
    if(freq.HaveMMX())
	zoom_2_32=zoom_2_32_mmx;
    else
	zoom_2_32=zoom_2_32_nommx;
    zoom_2_32(dest,src,w,h);
}

void (*v555to565)(uint16_t*, const uint16_t*, int, int)=v555to565_stub;
void (*zoom_2_16)(uint16_t*, const uint16_t*, int, int)=zoom_2_16_stub;
void (*zoom_2_16_to565)(uint16_t *, const uint16_t *, int,int)=zoom_2_16_to565_stub;
void (*zoom_2_32)(uint32_t *, const uint32_t *, int, int)=zoom_2_32_stub;

#else

void (*v555to565)(uint16_t*, const uint16_t*, int, int)=v555to565_nommx;
void zoom(uint16_t* dest, const uint16_t* src, int dst_w, int dst_h, int src_w, int src_h, int bpp, int xdim)
{
#warning ZOOM has to be written
}
#endif
