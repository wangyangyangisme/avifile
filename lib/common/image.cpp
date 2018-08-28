#include "image.h"
#include "avm_cpuinfo.h"
#include "mmx.h"
#include "avm_fourcc.h"
#include "avm_output.h"
#include "utils.h"
#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

AVM_BEGIN_NAMESPACE;

#define VALGRIND

#define STDCONV_PARAMS \
    CImage* to, const CImage* from, bool flip_dir

#define SHORTLINEARGS \
    uint8_t* dest, const uint8_t* src, int w

#define LONGLINEARGS \
    uint8_t* dest_y, uint8_t* dest_cr, uint8_t* dest_cb, \
    int ds_y, int ds_cr, int ds_cb, \
    const uint8_t* src_y, const uint8_t* src_cr, const uint8_t* src_cb, \
    int ss_y, int ss_cr, int ss_cb, \
    int w, int h

#define SETWHS() \
    int w = (to->m_Window.w < from->m_Window.w) ? to->m_Window.w : from->m_Window.w; \
    int h = (to->m_Window.h < from->m_Window.h) ? to->m_Window.h : from->m_Window.h; \
    int ss_y = (flip_dir) ? -from->m_iStride[0] : from->m_iStride[0]; \
    int ds_y = to->m_iStride[0]


#ifdef XXX_ARCH_X86
//MMX versions
#undef RENAME
#define HAVE_MMX
#undef HAVE_MMX2
#undef HAVE_3DNOW
#define RENAME(a) a ## _MMX
#include "image_mmx.h"

//MMX2 versions
#undef RENAME
#define HAVE_MMX
#define HAVE_MMX2
#undef HAVE_3DNOW
#define RENAME(a) a ## _MMX2
#include "image_mmx.h"
#endif

static inline void emms()
{
#if defined(ARCH_X86) && !defined(VALGRIND)
    if (freq.HaveMMX())
	__asm__ __volatile__ ("emms");
#endif
}

static void stride_memcpy(void* to, int strideout, const void* from, int stridein, int bpl, int h)
{
    //printf("Stride %p - %d  :::  %p  - %d   %d %d\n", out, strideout, in, stridein, bpl, h);
    if (
#ifdef VALGRIND
	1 // VALGRIND doesn't like MMX
#else
	bpl & 0x1f || !freq.HaveMMX()
#endif
       )
    {
	//printf("wrong h: %d  bpl: %d  %d %d\n", h, bpl, strideout, stridein);
	while (--h >= 0)
	{
	    memcpy(to, from, bpl);
	    to = (char*) to + strideout;
	    from =  (const char*) from + stridein;
	}
    }
#ifdef ARCH_X86
    // mmx2 doesnt write into l2 cache
    // XXX: sometimes this doesn't have to be a good idea so be careful here
    else if (freq.HaveMMXEXT())
    {
	//printf("stridecpy h: %d  bpl: %d  so:%d si:%d\n", h, bpl, strideout, stridein);
	char* tout = (char*)to + bpl;
	const char* tin = (const char*)from + bpl;
	while (--h >= 0)
	{
            int tbpl = -bpl;
	    __asm__ __volatile__
		(
		 ".balign 8			\n\t"
		 "1:				\n\t"
		 "movq   (%1, %0), %%mm0	\n\t"
		 "prefetchnta 32(%1, %0)	\n\t"
		 "movq  8(%1, %0), %%mm1	\n\t"
		 "movq 16(%1, %0), %%mm2	\n\t"
		 "movq 24(%1, %0), %%mm3	\n\t"
		 "prefetchnta 64(%1, %0)	\n\t"
		 "movntq %%mm0,   (%2, %0)	\n\t"
		 "movntq %%mm1,  8(%2, %0)	\n\t"
		 "movntq %%mm2, 16(%2, %0)	\n\t"
		 "movntq %%mm3, 24(%2, %0)	\n\t"
		 "addl $32, %0			\n\t"
                 "js   1b			\n\t"
		 :"+r"(tbpl)
		 :"r"(tin), "r"(tout)
		 :"memory"
		);
	    tout += strideout;
	    tin += stridein;
	}
    }
    else
    {
	//printf("stridecpy h: %d  bpl: %d  so:%d si:%d\n", h, bpl, strideout, stridein);
	char* tout = (char*)to + bpl;
	const char* tin = (const char*)from + bpl;
	while (--h >= 0)
	{
            int tbpl = -bpl;
	    __asm__ __volatile__
		(
		 ".balign 8			\n\t"
		 "1:				\n\t"
		 "movq   (%1, %0), %%mm0	\n\t"
		 "movq %%mm0,   (%2, %0)	\n\t"
		 "movq  8(%1, %0), %%mm1	\n\t"
		 "movq 16(%1, %0), %%mm2	\n\t"
		 "movq 24(%1, %0), %%mm3	\n\t"
		 "movq %%mm1,  8(%2, %0)	\n\t"
		 "movq %%mm2, 16(%2, %0)	\n\t"
		 "movq %%mm3, 24(%2, %0)	\n\t"
		 "addl $32, %0			\n\t"
                 "js   1b			\n\t"
		 :"+r"(tbpl)
		 :"r"(tin), "r"(tout)
		 :"memory"
		);
	    tout += strideout;
	    tin += stridein;
	}
    }
#endif
}

static void bgr32_bgr32_c(SHORTLINEARGS)
{
    stride_memcpy(dest, w * 4, src, w * 4, w * 4, 1);
}
static void bgr24_bgr24_c(SHORTLINEARGS)
{
    stride_memcpy(dest, w * 3, src, w * 3, w * 3, 1);
}
static void bgr16_bgr16_c(SHORTLINEARGS)
{
    stride_memcpy(dest, w * 2, src, w * 2, w * 2, 1);
}

static void bgr15_bgr16_c(SHORTLINEARGS)
{
    const uint16_t* s = (const uint16_t*) src;
    uint16_t* d = (uint16_t*) dest;
    while (w > 1)
    {
	w -= 2;
	unsigned q = *((const uint32_t *)&s[w]);
	*((uint32_t *)&d[w]) = (q & 0x7FFF7FFF) + (q & 0x7FE07FE0);
    }
    if (w)
    {
	uint16_t q = s[0];
	q += (q & 0x7FE0);
	d[0] = q;
    }
}
/*
 *  From BGR24 conversions
 */
static void bgr24_bgr15_c(SHORTLINEARGS)
{
    uint16_t* d = (uint16_t*) dest;
    while (--w >= 0)
	// looks ugly but gives good compiled code
	d[w] = ((((((src[w*3+2]<<5)&0xff00)|src[w*3+1])<<5)&0xfff00)|src[w*3+0])>>3;
}
static void bgr24_bgr16_c(SHORTLINEARGS)
{
    //printf("bgr24_bgr16_c %d\n", w);
    uint16_t* d = (uint16_t*)dest;
    while (--w >= 0)
	d[w] = ((((((src[w*3+2]<<5)&0xff00)|src[w*3+1])<<6)&0xfff00)|src[w*3+0])>>3;
}

/*
  test code - but speed is nearly the same as with version above

static void bgr24_bgr16_cl(SHORTLINEARGS, int h, int ds_y, int ss_y)
{
    while (h--)
    {
#if 0
	uint16_t* d = (uint16_t*) dest;
	uint16_t* e = (uint16_t*) dest + w;
	const uint8_t* s =  src;
        do {
	    // looks ugly but gives very fast compiled code
	    *d++ = ((((((s[2]<<5)&0xff00)|s[1])<<6)&0xfff00)|s[0])>>3;
	    s += 3;
	} while (d < e);
#else
	uint16_t* d = (uint16_t*)dest;
	for (int i = w - 1; i >= 0; i--)
	    d[i] = ((((((src[i*3+2]<<5)&0xff00)|src[i*3+1])<<6)&0xfff00)|src[i*3+0])>>3;
#endif
	dest += ds_y;
	src += ss_y;
    }
}
*/

static void bgr24_bgr32_c(SHORTLINEARGS)
{
    uint32_t* d = (uint32_t*) dest;
    while (w--)
    {
#ifndef WORDS_BIGENDIAN
        // FIXME - BIGENDIAN should be handled everywhere!!!
	*d++ = src[0] | (src[1] << 8) | (src[2] << 16); // b g r
#else
	*d++ = src[0] << 24 | (src[1] << 16) | (src[2] << 8); // b g r
	//dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2]; dest[3] = 0; dest += 4;
#endif
	src += 3;
    }
}

/*
 *  To BGR24 conversion
 */
static void bgr15_bgr24_c(SHORTLINEARGS)
{
    while (w--)
    {
	uint16_t sh = *(const uint16_t*)src;
	dest[0] = (sh & 0x1f) << 3; // b
	dest[1] = (sh & 0x3e0) >> 2; // g
	dest[2] = (sh & 0x7c00) >> 7; // r
	dest += 3;
	src += 2;
    }
}
static void bgr16_bgr24_c(SHORTLINEARGS)
{
    while (w--)
    {
	uint16_t sh = *(const uint16_t*)src;
	dest[0] = (sh & 0x1f) << 3; // b
	dest[1] = (sh & 0x7e0) >> 3; // g
	dest[2] = (sh & 0xf800) >> 8; // r
	src += 2;
	dest += 3;
    }
}
static void bgr32_bgr24_c(SHORTLINEARGS)
{
    while (w--)
    {
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
	src += 4;
	dest += 3;
    }
}

/*
 *  YUV conversions
 */
static void bgr24_yuv_c(SHORTLINEARGS)
{
    const col* s = (const col*) src;
    yuv* d = (yuv*) dest;
    while (w--)
	*d++ = *s++;
}
static void yuv_bgr24_c(SHORTLINEARGS)
{
    const yuv* s = (const yuv*) src;
    col* d = (col*) dest;

    while (w--)
	*d++ = *s++;
}

static void bgr24_yuy2_c(SHORTLINEARGS)
{
    const col* s = (const col*) src;
    uint32_t* d = (uint32_t*) dest;
    while (w--)
    {
	yuv yuvt(*s++);
	dest[0] = yuvt.Y;
	dest[1] = yuvt.Cb;
	dest[2] = s->Y();
	dest[3] = yuvt.Cr;

	s++;
	dest += 4;
    }
}
static void yuy2_bgr24_c(SHORTLINEARGS)
{
    col* d = (col*) dest;
    while (w--)
    {
	yuv p;

	p.Y = src[0];
	p.Cb = src[1];
	p.Cr = src[3];
	d[0] = p;
	p.Y = src[2];
	d[1] = p;

        src += 4;
        d += 2;
    }
}
static void uyvy_bgr24_c(SHORTLINEARGS)
{
    //printf("uyvy_bgr24_c %d\n", w);
    col* d = (col*) dest;
    while (w--)
    {
	yuv p;

	p.Cb = src[0];
	p.Y = src[1];
	p.Cr = src[2];
	d[0] = p;
	p.Y = src[3];
	d[1] = p;

        src += 4;
        d += 2;
    }
}

static void yv12_yuy2_c(LONGLINEARGS)
{
    //printf("yv12_yuy2_c\n");
    while (h--)
    {
	int i = w;
	while (--i >= 0)
	{
	    dest_y[i * 4] = src_y[i * 2];
	    dest_y[i * 4 + 2] = src_y[i * 2 + 1];
	    dest_y[i * 4 + ds_y] = src_y[i * 2 + ss_y];
	    dest_y[i * 4 + ds_y + 2] = src_y[i * 2 + ss_y + 1];

	    dest_y[i * 4 + 1] = dest_y[i * 4 + ds_y + 1] = src_cb[i];
	    dest_y[i * 4 + 3] = dest_y[i * 4 + ds_y + 3] = src_cr[i];
	}

	src_y += 2 * ss_y;
	src_cr += ss_cr;
	src_cb += ss_cb;

	dest_y += 2 * ds_y;
    }
}

static void yv12_bgr16_cSLOW(LONGLINEARGS)
{
    uint8_t l1[5000];
    uint8_t l2[5000];
  //printf("YV12 tgb16\n");
    while (h--)
    {
	//printf("H %d   %d %d\n", i, fromstride, tostride);
	col* dest = (col*) l1;
        col* dest2 = (col*) l2;
        int i;
	for (i = 0; i < w; i++)
	{
	    yuv p;
	    p.Cr = src_cr[i];
	    p.Cb = src_cb[i];
	    p.Y = src_y[i * 2];
	    dest[i * 2] = p;
	    p.Y = src_y[i * 2 + 1];
	    dest[i * 2 + 1] = p;
	    p.Y = src_y[i * 2 + ss_y];
	    dest2[i * 2] = p;
	    p.Y = src_y[i * 2 + ss_y + 1];
	    dest2[i * 2 + 1] = p;
	}

	bgr24_bgr16_c(dest_y, l1, w * 2);
	bgr24_bgr16_c(dest_y + ds_y, l2, w * 2);

	src_y += 2 * ss_y;
	src_cr += ss_cr;
	src_cb += ss_cb;

	dest_y += 2 * ds_y;
    }
}

static void yv12_bgr16_c(LONGLINEARGS)
{
    //printf("YV12 tgb16\n");
    while (h--)
    {
	//printf("H %d   %d %d\n", i, fromstride, tostride);
	uint16_t* dest = (uint16_t*) dest_y;
	uint16_t* dest2 = (uint16_t*) (dest_y + ds_y);
        int i;
	for (i = 0; i < w; i++)
	{
	    yuv p;
	    p.Cr = src_cr[i];
	    p.Cb = src_cb[i];
	    p.Y = src_y[i * 2];
	    dest[i * 2] = col(p).bgr16();
	    p.Y = src_y[i * 2 + 1];
	    dest[i * 2 + 1] = col(p).bgr16();
	    p.Y = src_y[i * 2 + ss_y];
	    dest2[i * 2] = col(p).bgr16();
	    p.Y = src_y[i * 2 + ss_y + 1];
	    dest2[i * 2 + 1] = col(p).bgr16();
	}

	src_y += 2 * ss_y;
	src_cr += ss_cr;
	src_cb += ss_cb;

	dest_y += 2 * ds_y;
    }
}

static void yv12_bgr24_c(LONGLINEARGS)
{
    while (h--)
    {
	//printf("H %d   %d %d\n", i, fromstride, tostride);
	col* dest = (col*) dest_y;
        col* dest2 = (col*) (dest_y + ds_y);
        int i;
	for (i = 0; i < w; i++)
	{
	    yuv p;
	    p.Cr = src_cr[i];
	    p.Cb = src_cb[i];
	    p.Y = src_y[i * 2];
	    dest[i * 2] = p;
	    p.Y = src_y[i * 2 + 1];
	    dest[i * 2 + 1] = p;
	    p.Y = src_y[i * 2 + ss_y];
	    dest2[i * 2] = p;
	    p.Y = src_y[i * 2 + ss_y + 1];
	    dest2[i * 2 + 1] = p;
	}

	src_y += 2 * ss_y;
	src_cr += ss_cr;
	src_cb += ss_cb;

	dest_y += 2 * ds_y;
    }
}

static void y800_bgr24_c(LONGLINEARGS)
{
    printf("HxW %dx%d  ssy:%d  dsy:%d\n", h, w, ss_y, ds_y);
    while (h--)
    {
	col* dest = (col*) dest_y;
        int i;
	for (i = 0; i < w; i++)
	{
	    yuv p;
	    p.Cr = 0x80;
	    p.Cb = 0x80;
	    p.Y = src_y[i];
	    dest[i] = p;
	}

	src_y += ss_y;
	dest_y += ds_y;
    }
}

static void bgr24_yv12_c(LONGLINEARGS)
{
    //printf("bgr24ToYv12  %d   %d\n", w, h);
    while (h--)
    {
	const uint8_t* s = src_y;
	uint8_t* dest_y1 = dest_y + ds_y;
	for (int i = 0; i < w; i++)
	{
	    yuv yuvt(*(const col*)s);
	    dest_cb[i] = yuvt.Cb;
	    dest_cr[i] = yuvt.Cr;
	    dest_y[i * 2] = yuvt.Y;
	    dest_y[i * 2 + 1] = ((const col*)(s + 3))->Y();
	    dest_y1[i * 2] = ((const col*)(s + ss_y))->Y();
	    dest_y1[i * 2 + 1] = ((const col*)(s + ss_y + 3))->Y();

	    s += 6;
	}

        src_y += 2 * ss_y;

        dest_y += 2 * ds_y;
	dest_cr += ds_cr;
	dest_cb += ds_cb;
    }
}

static void yuy2_yv12_c(LONGLINEARGS)
{
    //printf("yuy2ToYv12\n");
    while (h--)
    {
	uint8_t* d1 = dest_y;
	uint8_t* d2 = dest_y + ds_y;
        int i = w;
	while (--i >= 0)
	{
	    d1[i * 2] = src_y[i * 4];
	    dest_cb[i] = src_y[i * 4 + 1];
	    d2[i * 2] = src_y[i * 4 + ss_y];
	    d1[i * 2 + 1] = src_y[i * 4 + 2];
	    d2[i * 2 + 1] = src_y[i * 4 + 2 + ss_y];
	    dest_cr[i] = src_y[i * 4 + 3];
	}
	src_y += 2 * ss_y;

	dest_y += 2 * ds_y;
        dest_cr += ds_cr;
	dest_cb += ds_cb;
    }
}

static void uyvy_yv12_c(LONGLINEARGS)
{
    //printf("uyvyToYv12\n");
    while (h--)
    {
	uint8_t* d1 = dest_y;
	uint8_t* d2 = dest_y + ds_y;
        int i = w;
	while (--i >= 0)
	{
	    dest_cb[i] = src_y[i * 4];
	    d1[i * 2] = src_y[i * 4 + 1];
	    d2[i * 2] = src_y[i * 4 + ss_y + 1];
	    dest_cr[i] = src_y[i * 4 + 2];
	    d1[i * 2 + 2] = src_y[i * 4 + 3];
	    d2[i * 2 + 2] = src_y[i * 4 + 3 + ss_y];
	}
	src_y += 2 * ss_y;

	dest_y += 2 * ds_y;
        dest_cr += ds_cr;
	dest_cb += ds_cb;
    }
}

static void i420_yv12_c(LONGLINEARGS)
{
    //printf("i420ToYv12\n");
    stride_memcpy(dest_y, ds_y, src_y, ss_y, w, h);
    // src_cr & src_cb are already swapped when this
    // routine is being called - that's why it looks like
    // pure copy
    stride_memcpy(dest_cr, ds_cr, src_cr, ss_cr, w / 2, h / 2);
    stride_memcpy(dest_cb, ds_cb, src_cb, ss_cb, w / 2, h / 2);
}

static void i411_yv12_c(LONGLINEARGS)
{
    // mainly ffmpeg mjpeg
    //printf("i411ToYv12\n");
    stride_memcpy(dest_y, ds_y, src_y, ss_y, w, h);

    h /= 2;
    w /= 4;

    while (h--)
    {
        int i = w;
	while (--i >= 0)
	{
            dest_cb[i * 2] = src_cr[i];
	    dest_cr[i * 2] = src_cb[i];
            // for next pixels use pixels from the next line
	    dest_cb[i * 2 + 1] = src_cr[i + ss_cr];
	    dest_cr[i * 2 + 1] = src_cb[i + ss_cb];
	}

        src_cr += 2 * ss_cr;
        src_cb += 2 * ss_cb;

	dest_cr += ds_cr;
	dest_cb += ds_cb;
    }
}

static void i422_yv12_c(LONGLINEARGS)
{
    // mainly ffmpeg mjpeg
    //printf("i422ToYv12 %d %p %d %d %d %d  %d\n", ds_y, src_y, ss_y, w, h, ss_cb, ss_cr);
    stride_memcpy(dest_y, ds_y, src_y, ss_y, w, h);
    stride_memcpy(dest_cr, ds_cr, src_cb, 2 * ss_cb, w / 2, h / 2);
    stride_memcpy(dest_cb, ds_cb, src_cr, 2 * ss_cr, w / 2, h / 2);
}

static void i444_yv12_c(LONGLINEARGS)
{
    // another mjpeg output from ffmpeg
    //printf("i444ToYv12\n");
    stride_memcpy(dest_y, ds_y, src_y, ss_y, w, h);
    h /= 2;
    w /= 2;

    while (h--)
    {
        int i = w;
	while (--i >= 0)
	{
            dest_cb[i] = src_cr[i * 2];
            dest_cr[i] = src_cb[i * 2];
	}

        src_cr += 2 * ss_cr;
        src_cb += 2 * ss_cb;

	dest_cr += ds_cr;
	dest_cb += ds_cb;
    }
}

static void i410_yv12_c(LONGLINEARGS)
{
    // mainly ffmpeg indeo
    //printf("i410ToYv12\n");
    stride_memcpy(dest_y, ds_y, src_y, ss_y, w, h);

    h /= 4;
    w /= 4;

    while (h--)
    {
        int i = w;
	while (--i >= 0)
	{
	    dest_cr[i * 2] = dest_cr[i * 2 + 1] = src_cb[i];
            dest_cb[i * 2] = dest_cb[i * 2 + 1] = src_cr[i];
            // for next pixels use pixels from the next line
	    dest_cr[i * 2 + ds_cr] = dest_cr[i * 2 + 1 + ds_cr] = src_cb[i];
            dest_cb[i * 2 + ds_cb] = dest_cb[i * 2 + 1 + ds_cb] = src_cr[i];
	    //dest_cb[i * 2 + 1] = src_cr[i + ss_cr];
	    //dest_cr[i * 2 + 1] = src_cb[i + ss_cb];
	}

        src_cr += ss_cr;
        src_cb += ss_cb;

	dest_cr += 2 * ds_cr;
	dest_cb += 2 * ds_cb;
    }
}

static void y800_yv12_c(LONGLINEARGS)
{
    printf("y800ToYv12 not yet implemented\n");
    return;
    stride_memcpy(dest_y, ds_y, src_y, ss_y, w, h);

    h /= 4;
    w /= 4;

    while (h--)
    {
	memset(dest_cr, 0x80, w); dest_cr += ds_cr;
	memset(dest_cb, 0x80, w); dest_cb += ds_cb;
    }
}

enum {
    // short
    BGR32TOBGR32,
    BGR24TOBGR24,
    BGR16TOBGR16,

    BGR15TOBGR16,

    BGR24TOBGR15,
    BGR15TOBGR24,

    BGR24TOBGR16,
    BGR16TOBGR24,

    BGR24TOBGR32,
    BGR32TOBGR24,

    BGR24TOYUV,
    YUVTOBGR24,

    BGR24TOYUV2,
    YUV2TOBGR24,

    UYVYTOBGR24,
    Y800TOBGR24,

    MAXSHORT,

    // long
    BGR24TOYV12,
    YV12TOBGR24,

    //BGR24TOYV12,
    YV12TOBGR16,

    YV12TOYUY2,
    YUY2TOYV12,

    UYVYTOYV12,

    I420TOYV12,
    I411TOYV12,
    I422TOYV12,
    I444TOYV12,
    I410TOYV12,
    Y800TOYV12,

    MAXLONG
};

#define V(a)  (void*)a
// mapping table - order hs to match above enum
static const struct lconvmap_s {
    int div_w;
    void* s_c;
    void* s_mmx;
    void* s_mmx2;
} lconvmap[] = {

    // short options (only width is divided)
    { 1, V(bgr32_bgr32_c), V(bgr32_bgr32_c), V(bgr32_bgr32_c) },
    { 1, V(bgr24_bgr24_c), V(bgr24_bgr24_c), V(bgr24_bgr24_c) },
    { 1, V(bgr16_bgr16_c), V(bgr16_bgr16_c), V(bgr16_bgr16_c) },

    { 1, V(bgr15_bgr16_c), V(bgr15_bgr16_c), V(bgr15_bgr16_c) },

    { 1, V(bgr24_bgr15_c), V(bgr24_bgr15_c), V(bgr24_bgr15_c) },
    { 1, V(bgr15_bgr24_c), V(bgr15_bgr24_c), V(bgr15_bgr24_c) },

    { 1, V(bgr24_bgr16_c), V(bgr24_bgr16_c), V(bgr24_bgr16_c) },
    { 1, V(bgr16_bgr24_c), V(bgr16_bgr24_c), V(bgr16_bgr24_c) },

    { 1, V(bgr24_bgr32_c), V(bgr24_bgr32_c), V(bgr24_bgr32_c) },
    { 1, V(bgr32_bgr24_c), V(bgr32_bgr24_c), V(bgr32_bgr24_c) },

    { 1, V(bgr24_yuv_c), V(bgr24_yuv_c), V(bgr24_yuv_c) },
    { 1, V(yuv_bgr24_c), V(yuv_bgr24_c), V(yuv_bgr24_c) },

    { 2, V(bgr24_yuy2_c), V(bgr24_yuy2_c), V(bgr24_yuy2_c) },
    { 2, V(yuy2_bgr24_c), V(yuy2_bgr24_c), V(yuy2_bgr24_c) },

    { 2, V(uyvy_bgr24_c), V(uyvy_bgr24_c), V(uyvy_bgr24_c) },
    { 1, V(y800_bgr24_c), V(y800_bgr24_c), V(y800_bgr24_c) },

    { 0 },

    // long options
    { 2, V(bgr24_yv12_c), V(bgr24_yv12_c), V(bgr24_yv12_c) },
    { 2, V(yv12_bgr24_c), V(yv12_bgr24_c), V(yv12_bgr24_c) },

    { 2, V(yv12_bgr16_c), V(yv12_bgr16_c), V(yv12_bgr16_c) },

    { 2, V(yv12_yuy2_c), V(yv12_yuy2_c), V(yv12_yuy2_c) },
    { 2, V(yuy2_yv12_c), V(yuy2_yv12_c), V(yuy2_yv12_c) },

    { 2, V(uyvy_yv12_c), V(uyvy_yv12_c), V(uyvy_yv12_c) },

    { 1, V(i420_yv12_c), V(i420_yv12_c), V(i422_yv12_c) },
    { 1, V(i411_yv12_c), V(i411_yv12_c), V(i411_yv12_c) },
    { 1, V(i422_yv12_c), V(i422_yv12_c), V(i422_yv12_c) },
    { 1, V(i444_yv12_c), V(i444_yv12_c), V(i444_yv12_c) },
    { 1, V(i410_yv12_c), V(i410_yv12_c), V(i410_yv12_c) },
    { 1, V(y800_yv12_c), V(y800_yv12_c), V(y800_yv12_c) },
};
#undef V

/*
 * generic conversion for 1plane routines
 */
static void lineconvert(STDCONV_PARAMS, unsigned type)
{
    typedef void (*LCONV)(SHORTLINEARGS);
    LCONV lconv;
    //flip_dir = true;

    const uint8_t* src = from->GetWindow(0);
    uint8_t* dest = to->GetWindow(0);
    SETWHS();

    if (flip_dir)
	src += (from->m_Window.h - 1) * from->Stride();
    //printf("W %d   %d   %d  w: %d   flip: %d\n", from->Width(), from->Stride(), from->m_Window.h, w, flip_dir);
    w /= lconvmap[type].div_w;

    assert(type < MAXSHORT);
    lconv = (LCONV) lconvmap[type].s_c;
#if 1
    if (w == to->Width() && 1
	&& w == from->Width()
	&& to->Bpl() == ds_y
	&& from->Bpl() == ss_y)
    {
        // one stright call is enough in this case
        lconv(dest, src, w * h);
    }
    else
	while (h--)
	{
	    //printf("DEST %p  %p   %d  ds: %d ss: %d   %d\n", dest, src, to->Stride(), ds_y, ss_y, h);
	    lconv(dest, src, w);
	    dest += ds_y;
	    src += ss_y;
	}
#else
    bgr24_bgr16_cl(dest, src, w, h, ds_y, ss_y);
#endif
    emms();
}

static void yuvconv(STDCONV_PARAMS, unsigned type)
{
    typedef void (*LCONV)(LONGLINEARGS);
    LCONV lconv;
    //flip_dir = true;

    const uint8_t* src_y = from->GetWindow(0);
    const uint8_t* src_cr = from->GetWindow(1);
    const uint8_t* src_cb = from->GetWindow(2);

    SETWHS();
    w = (w + 1) & ~1;
    int ss_cr;
    int ss_cb;
    //printf("YUVCON  %d  %d   %d %d   %d\n", w, h, from->m_Window.x, from->m_Window.y, to->m_Window.x);

    if (flip_dir)
    {
	src_y += (from->m_Window.h - 1) * from->m_iStride[0];
	src_cr += (from->m_Window.h / lconvmap[type].div_w - 1) * from->m_iStride[1];
	src_cb += (from->m_Window.h / lconvmap[type].div_w - 1) * from->m_iStride[2];
        ss_cr = -from->m_iStride[1];
        ss_cb = -from->m_iStride[2];
    }
    else
    {
	ss_cr = from->m_iStride[1];
	ss_cb = from->m_iStride[2];
    }

    if (from->Format() == IMG_FMT_I420)
    {
	// swap U V
	const uint8_t* t = src_cb;
	int s = ss_cb;
	src_cb = src_cr;
	src_cr = t;
	ss_cb = ss_cr;
	ss_cr = s;
    }

    int swds = (to->Format() == IMG_FMT_I420) ? 1 : 0;

    lconv = (LCONV) lconvmap[type].s_c;
    w /= lconvmap[type].div_w;
    h /= lconvmap[type].div_w;
    lconv(to->GetWindow(0), to->GetWindow(1 + swds), to->GetWindow(2 - swds),
	  to->m_iStride[0], to->m_iStride[1 + swds], to->m_iStride[2 - swds],
	  src_y, src_cr, src_cb, ss_y, ss_cr, ss_cb, w, h);
    emms();
}


static void anyFromRgb24(STDCONV_PARAMS)
{
    switch (to->Format())
    {
    case 15:
	lineconvert(to, from, flip_dir, BGR24TOBGR15);
	return;
    case 16:
	lineconvert(to, from, flip_dir, BGR24TOBGR16);
	return;
    case 24:
	lineconvert(to, from, flip_dir, BGR24TOBGR24);
	return;
    case 32:
	lineconvert(to, from, flip_dir, BGR24TOBGR32);
	return;
    case IMG_FMT_YUV:
	lineconvert(to, from, flip_dir, BGR24TOYUV);
	return;
    case IMG_FMT_YUY2:
	lineconvert(to, from, flip_dir, BGR24TOYUV2);
	return;
    case IMG_FMT_YV12:
	yuvconv(to, from, flip_dir, BGR24TOYV12);
	return;
    }

    int f = to->Format();
    AVM_WRITE("CImage", "Cannot convert from 24 bit image to unimplemented %.4s  0x%x\n",
	      (const char*)&f, f);
}

static void anyToRgb24(STDCONV_PARAMS)
{
    switch(from->Format())
    {
    case 15:
	lineconvert(to, from, flip_dir, BGR15TOBGR24);
	return;
    case 16:
	lineconvert(to, from, flip_dir, BGR16TOBGR24);
	return;
    case 24:
	lineconvert(to, from, flip_dir, BGR24TOBGR24);
	return;
    case 32:
	lineconvert(to, from, flip_dir, BGR32TOBGR24);
	return;
    case IMG_FMT_YUV:
	lineconvert(to, from, flip_dir, YUVTOBGR24);
	return;
    case IMG_FMT_YUY2:
	lineconvert(to, from, flip_dir, YUV2TOBGR24);
	return;
    case IMG_FMT_Y422:
    case IMG_FMT_UYVY:
	lineconvert(to, from, flip_dir, UYVYTOBGR24);
	return;
    case IMG_FMT_YV12:
    case IMG_FMT_I420:
	// handles both yv12 & i420
	yuvconv(to, from, flip_dir, YV12TOBGR24);
	return;;
    case IMG_FMT_Y800:
	// handles both yv12 & i420
	yuvconv(to, from, flip_dir, Y800TOBGR24);
	return;;
    }

    int f = from->Format();
    AVM_WRITE("CImage", "Cannot convert to 24 bit image from unimplemented %.4s  0x%x\n",
	      (const char*)&f, f);
}

lookuptable col::t = lookuptable();

lookuptable::lookuptable()
{
    for(int i = 255; i >= 0; i--)
    {
	m_plY[i] = 298 * (i - 16);
	m_plRV[i] = 408 * (i - 128);
	m_plGV[i] = -208 * (i - 128);
	m_plGU[i] = -100 * (i - 128);
	m_plBU[i] = 517 * (i - 128);
    }
}


/**
 * Creates new image in format 'header' from specified memory area.
 * Either allocates its own memory area & copies src data into it, or reuses
 * parent data.
 */
CImage::CImage(const BitmapInfo* header, const uint8_t* data, bool copy)
    :m_pInfo(header)
{
    fillMembers();
    if (!copy)
    {
	m_pPlane[0] = (uint8_t*) data;
    }
    else
    {
        int s = m_iBytes[0] + m_iBytes[1] + m_iBytes[2] + m_iBytes[3];
	m_pPlane[0] = new uint8_t[s];
	m_bDataOwner[0] = true;
	if (data)
	    memcpy(m_pPlane[0], data, s);
    }
    setPlanes();
    //printf("COMPRESSTRIDE %4s\n", (char*)&m_iFormat);
}

CImage::CImage(const BitmapInfo* header, const uint8_t* plane[], const int stride[], bool copy)
    :m_pInfo(header)
{
    //printf("CONST2\n");
    fillMembers();

    if (!copy)
    {
	memcpy(m_pPlane, plane, sizeof(uint8_t*) * 3);
	memcpy(m_iStride, stride, sizeof(int) * 3);
	//printf("CIMAGE %p %p %p\n", m_pPlane[0], m_pPlane[1], m_pPlane[2]);
	//printf("CIMAGE %d %d %d\n", m_iStride[0], m_iStride[1], m_iStride[2]);
    }
    else
    {
	m_pPlane[0] = new uint8_t[m_iBytes[0] + m_iBytes[1] + m_iBytes[2]];
	m_bDataOwner[0] = true;
	for (int i = 0; i < 3; i++)
	{
	    if (m_iBytes[i] && plane[i])
		stride_memcpy(m_pPlane[i], m_iStride[i],
			      plane[i], stride[i], m_iWidth, m_iHeight);
	}
	emms();
    }
    setPlanes();
}

/* Creates 24-bit RGB image from 24-bit RGB 'data' */
CImage::CImage(const uint8_t* data, int width, int height)
    :m_pInfo(BitmapInfo(width, height, 24))
{
    fillMembers();
    m_pPlane[0] = new uint8_t[m_iBytes[0]];
    m_bDataOwner[0] = true;
    if (data)
	memcpy(m_pPlane[0], data, m_iBytes[0]);
}

// non-conversion constructors
CImage::CImage(const CImage* im)
    :m_pInfo(im->GetFmt())
{
    fillMembers();
    m_fQuality = im->GetQuality();
    int s = m_iBytes[0] + m_iBytes[1] + m_iBytes[2] + m_iBytes[3];

    m_pPlane[0] = new uint8_t[s];
    m_iStride[0] = im->m_iStride[0];
    m_iBytes[0] = im->m_iBytes[0];
    m_bDataOwner[0] = true;
    memcpy(m_pPlane[0], im->m_pPlane[0], m_iBytes[0]);

    for (unsigned i = 1; i < CIMAGE_MAX_PLANES; i++)
    {
	m_iStride[i] = im->m_iStride[i];
	m_iBytes[i] = im->m_iBytes[i];
        m_pPlane[i] = m_pPlane[i - 1] + m_iBytes[i - 1];
	memcpy(m_pPlane[i], im->m_pPlane[i], m_iBytes[i]);
	//printf("COPY STRIDE  %i  %p %d  %d\n", i, m_pPlane[i], m_iStride[i], m_iBytes[i]);
	//printf("ALLOCPLANE %p(%d): %p\n", this, i, m_pPlane[i]);
    }
}

// Conversion constructors
CImage::CImage(const CImage* im, uint_t csp_bits)
    :m_pInfo(im->GetFmt())
{
    if (csp_bits <= 32)
	m_pInfo.SetBits(csp_bits);
    else
        m_pInfo.SetSpace(csp_bits);
    fillMembers();
    m_fQuality = im->GetQuality();
    int s = m_iBytes[0] + m_iBytes[1] + m_iBytes[2] + m_iBytes[3];
    m_pPlane[0] = new uint8_t[s];
    m_bDataOwner[0] = true;
    setPlanes();
    Convert(im);
}

CImage::CImage(const CImage* im, const BitmapInfo* header)
    :m_pInfo(header)
{
    fillMembers();
    m_fQuality = im->GetQuality();
    int s = m_iBytes[0] + m_iBytes[1] + m_iBytes[2] + m_iBytes[3];
    m_pPlane[0] = new uint8_t[s];
    m_bDataOwner[0] = true;
    setPlanes();
    Convert(im);
}

CImage::~CImage()
{
    //printf("CIMDELETE %p cnt: %d  own:%d  (%p %p %p\n", this, m_iRefcount, m_bDataOwner, m_pPlane[0],m_pPlane[1],m_pPlane[2]);
    if (m_iRefcount > 1)
    {
	AVM_WRITE("CImage", "Unexpected delete of referenced image ! (%d) (USE RELEASE)\n", m_iRefcount);
    }
    for (uint_t i = 0; i < CIMAGE_MAX_PLANES; i++)
	if (m_bDataOwner[i])
	    delete[] m_pPlane[i];
}

void CImage::fillMembers()
{
    m_iRefcount = 1;
    m_pUserData = 0;
    m_pAllocator = 0;
    m_fQuality = 0.0f;
    m_iType = 0;
    m_fAspectRatio = 1.0;
    for (unsigned i = 0; i < CIMAGE_MAX_PLANES; i++)
    {
	m_pPlane[i] = 0;
	m_iStride[i] = 0;
        m_iBytes[i] = 0;
	m_bDataOwner[i] = false;
    }
    // setup most common values
    // for some colorspaces these HAVE TO BE FIXED
    m_iDepth = m_pInfo.biBitCount;
    m_iFormat = m_pInfo.biCompression;
    if (m_pInfo.IsRGB())
        m_iFormat = m_pInfo.Bpp();

    m_iBpp = (m_iDepth + 7) / 8;
    m_iBpl = m_iBpp * m_pInfo.biWidth;
    m_Window.x = m_Window.y = 0;
    m_iWidth = m_Window.w = m_pInfo.biWidth;
    m_iHeight = m_Window.h = labs(m_pInfo.biHeight);
    m_iPixels = m_iWidth * m_iHeight;
    m_iBytes[0] = m_iBpl * m_iHeight;
    m_iStride[0] = m_iBpl;

    switch (m_iFormat)
    {
    case IMG_FMT_YV12:
    case IMG_FMT_I420:
        m_iBpp = 1;
	m_iStride[0] = m_iBpl = m_iWidth;
	m_iStride[1] = m_iStride[2] = m_iWidth / 2;
	m_iBytes[0] = m_iStride[0] * m_iHeight;
	m_iBytes[1] = m_iStride[1] * m_iHeight / 2;
	m_iBytes[2] = m_iStride[2] * m_iHeight / 2;
	break;
    case IMG_FMT_I422:
        m_iBpp = 1;
	m_iStride[0] = m_iBpl = m_iWidth;
	m_iStride[1] = m_iStride[2] = m_iWidth / 2;
	m_iBytes[0] = m_iStride[0] * m_iHeight;
	m_iBytes[1] = m_iStride[1] * m_iHeight;
	m_iBytes[2] = m_iStride[2] * m_iHeight;
	break;
    case IMG_FMT_I444:
        m_iBpp = 1;
	m_iStride[0] = m_iStride[1] = m_iStride[2] = m_iBpl = m_iWidth;
	m_iBytes[0] = m_iStride[0] * m_iHeight;
	m_iBytes[1] = m_iStride[1] * m_iHeight;
	m_iBytes[2] = m_iStride[2] * m_iHeight;
	break;
    case IMG_FMT_YUY2:
    case IMG_FMT_UYVY:
	m_iBpp = 2;
	m_iBpl = m_iBpp * m_pInfo.biWidth;
        break;
    }
}

void CImage::setPlanes()
{
    switch (m_iFormat)
    {
    case IMG_FMT_YV12:
    case IMG_FMT_I420:
    case IMG_FMT_I422:
    case IMG_FMT_I444:
	if (!m_pPlane[1])
	    m_pPlane[1] = m_pPlane[0] + m_iBytes[0];
        if (!m_pPlane[2])
	    m_pPlane[2] = m_pPlane[0] + m_iBytes[0] + m_iBytes[1];
	break;
    }
}

void CImage::Release() const
{
    //printf("CIMRELEASE %p cnt: %d  own:%d  p0:%p\n", this, m_iRefcount, m_bDataOwner, m_pPlane[0]);
    m_iRefcount--;
    if (!m_iRefcount)
	delete this;
}

void CImage::ToYUV()
{
    if (m_pInfo.biCompression != 0)
	return;

    yuv* src;
    src = (yuv*)Data()  + m_iPixels - 1;
    if (m_iDepth != 24)
    {
	AVM_WRITE("CImage", "Cannot convert non-24 bit image to YUV\n");
	return;
    }
    while ((uint8_t*)src > Data() + 3)
    {
	*src = *(col*) src;
	src--;
	*src = *(col*) src;
	src--;
	*src = *(col*) src;
	src--;
	*src = *(col*) src;
	src--;
    }
    m_iFormat = m_pInfo.biCompression = IMG_FMT_YUV;
}

void CImage::ToRGB()
{
    if (m_pInfo.biCompression != IMG_FMT_YUV || m_iDepth != 24)
    {
	AVM_WRITE("CImage", "Cannot convert non-YUV image to BGR24\n");
	return;
    }
    col* src= (col*)Data() + m_iPixels - 1;
    while ((uint8_t*)src > Data() + 3)
    {
	*src= *(struct yuv*) src;
	src--;
	*src= *(struct yuv*) src;
	src--;
	*src= *(struct yuv*) src;
	src--;
	*src= *(struct yuv*) src;
	src--;
    }
    //m_pInfo.SetBits(24);
    m_iFormat = m_pInfo.biCompression = 0;
}

void CImage::Blur(int range, int from)
{
    if(range<=0)return;
    if(from<0)return;
    if(range<=from)return;

#ifdef ARCH_X86
    int limit = m_iPixels * 3;
    unsigned lim=1<<(range+1);
    unsigned ach=3*(1<<(from+1));
    uint8_t* p = Data();
    while (ach < 3 * lim)
    {
	//printf("Blur  %d\n", ach);
	asm volatile
	    (
	     //p %0
	     //ach %1
	     //limit %2
	     "pushl %%eax	\n\t"
	     "pushl %%ebx	\n\t"
	     "pushl %%ecx	\n\t"
	     "pushl %%edx	\n\t"
	     "pushl %%edi	\n\t"

	     "movl %0, %%ebx	\n\t"

	     "movl %1, %%ecx	\n\t"

	     "movl %0, %%edx	\n\t"
	     "addl %2, %%edx	\n\t"
	     "subl %1, %%edx	\n\t"

	     "movl %%edx, %%edi	\n\t"

	     "pxor %%mm2, %%mm2	\n\t"

	     "xorl %%eax, %%eax	\n\t"
	     "xorl %%edx, %%edx	\n\t"
	     //	align 8
	     "1:		\n\t"
	     "movd (%%ebx), %%mm0\n\t"
	     //	"movd %%ebx(%%ecx), %%mm1 \n\t"
	     "movd (%%ecx,%%ebx), %%mm1 \n\t"
	     "punpcklbw %%mm2, %%mm1\n\t"
	     "punpcklbw %%mm2, %%mm0\n\t"
	     "paddusw %%mm1, %%mm0 \n\t"
	     "psrlw $1, %%mm0\n\t"
	     "packuswb %%mm2, %%mm0\n\t"
	     "movd  %%mm0, (%%ebx)\n\t"

	     "addl $4, %%ebx	\n\t"
	     "cmpl %%edi, %%ebx	\n\t"
	     "jb 1b		\n\t"

	     "popl %%edi	\n\t"
	     "popl %%edx	\n\t"
	     "popl %%ecx	\n\t"
	     "popl %%ebx	\n\t"
	     "popl %%eax	\n\t"
	     :
	     :"r"(p), "r"(ach), "r"(limit)
	    );
	ach*=m_iWidth;
	asm volatile
	    (
	     //p %0
	     //ach %1
	     //limit %2
	     "pushl %%eax	\n\t"
	     "pushl %%ebx	\n\t"
	     "pushl %%ecx	\n\t"
	     "pushl %%edx	\n\t"
	     "pushl %%edi	\n\t"
	     "movl %0, %%ebx	\n\t"
	     "movl %0, %%ebx	\n\t"
	     "movl %1, %%ecx	\n\t"
	     "movl %0, %%edx	\n\t"
	     "addl %2, %%edx	\n\t"
	     "subl %1, %%edx	\n\t"

	     "movl %%edx, %%edi	\n\t"

	     "pxor %%mm2, %%mm2	\n\t"

	     "xorl %%eax, %%eax	\n\t"
	     "xorl %%edx, %%edx	\n\t"
	     //			align 8
	     "1:		\n\t"
	     "movd (%%ebx), %%mm0\n\t"
	     "movd (%%ebx,%%ecx), %%mm1 \n\t"
	     "punpcklbw %%mm2, %%mm1\n\t"
	     "punpcklbw %%mm2, %%mm0\n\t"
	     "paddusw %%mm1, %%mm0\n\t"
	     "psrlw $1, %%mm0\n\t"
	     "packuswb %%mm2, %%mm0\n\t"
	     "movd  %%mm0, (%%ebx)\n\t"

	     "addl $4, %%ebx	\n\t"
	     "cmpl %%edi, %%ebx	\n\t"
	     "jb 1b		\n\t"
	     "popl %%edi	\n\t"
	     "popl %%edx	\n\t"
	     "popl %%ecx	\n\t"
	     "popl %%ebx	\n\t"
	     "popl %%eax	\n\t"
	     :
	     :"r"(p), "r"(ach), "r"(limit)
	    );
	ach/=m_iWidth;
	ach*=2;
    }
    emms();
#else
#warning BLUR not implemented in C code
#endif
}

bool CImage::Supported(int csp, int bitcount)
{
    //AVM_WRITE("CImage", "SUPPORTED 0x%x (%.4s)  %d\n", csp, (char*)&csp, bitcount);
    if (csp == 0)
    {
	switch(bitcount)
	{
	case 15:
	case 16:
	case 24:
	case 32:
	    return true;
	}
    }
    else if (csp == 3)
    {
	switch(bitcount)
	{
	case 15:
	case 16:
	    return true;
	}
    }
    else
    {
	switch (StandardFOURCC(csp))
	{
	case IMG_FMT_YUV:
	case IMG_FMT_YUY2:
	case IMG_FMT_UYVY:
	case IMG_FMT_YV12:
	case IMG_FMT_I420:
	case IMG_FMT_Y422:
	case IMG_FMT_I422:
	case IMG_FMT_I444:
	    //case IMG_FMT_YVYU:
	    //case IMG_FMT_IYUV:
	    return true;
	}
    }
    return false; // unsupported
}

bool CImage::Supported(const BITMAPINFOHEADER& head)
{
    return Supported(head.biCompression, head.biBitCount);
}

fourcc_t CImage::StandardFOURCC(fourcc_t csp)
{
    switch (csp)
    {
    case IMG_FMT_V422:
    case IMG_FMT_YUNV:
	return IMG_FMT_YUY2;
    case IMG_FMT_IYUV:
	return IMG_FMT_I420;
    case IMG_FMT_IYU2:
    case IMG_FMT_Y422:
	return IMG_FMT_UYVY;
    case IMG_FMT_Y8:
	return IMG_FMT_Y800;
    }
    return csp;
}

void CImage::ByteSwap()
{
    if (m_pInfo.biCompression != 0 || m_pInfo.biBitCount != 24)
	return;

    uint8_t* t = Data();
    uint8_t* e = Data() + m_iPixels * 3 - 11;
    while (t < e)
    {
	uint8_t tmp = t[0];
	t[0] = t[2];
	t[2] = tmp;

	uint8_t tmp1 = t[3];
	t[3] = t[5];
	t[5] = tmp1;

	uint8_t tmp2 = t[6];
	t[6] = t[8];
	t[8] = tmp2;

	uint8_t tmp3 = t[9];
	t[9] = t[11];
	t[11] = tmp3;

	t += 12;
    }
}

void CImage::Dump(const char* filename)
{
    int fd;
#ifndef WIN32
    fd=open(filename, O_WRONLY|O_CREAT|O_TRUNC, 00666);
#else
    fd=open(filename, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 00666);
#endif
    if(fd<0)
    {
	AVM_WRITE("CImage", "Warning: could not open %s for writing", filename);
	return;
    }

    //    int w=im->width();
    //    int h=im->height();
    BitmapInfo bi(m_iWidth, m_iHeight, 24);
    CImage* im=0;
    if (!IsFmt(&bi))
	im = new CImage(this, &bi);
    const uint8_t* ptr = (im) ? im->Data() : Data();
    int bs = m_iPixels * 3;
    uint16_t bfh[7];
    bfh[0] = 'B' + 256 * 'M';
    *(int*)&bfh[1]=bs+0x36;
    *(int*)&bfh[3]=0;
    *(int*)&bfh[5]=0x36;
    write(fd, bfh, 14);
    write(fd, &bi, 40);
    write(fd, ptr, bs);
    close(fd);
    if (im)
	im->Release();
}

void CImage::Clear()
{
    //AVM_WRITE("CImage", "CLEAR IMAGE 0x%x %.4s %d,%d,%d\n", Format(), m_iBytes[0], m_iBytes[1], m_iBytes[2]);
    switch (StandardFOURCC(Format()))
    {
    case IMG_FMT_YV12:
    case IMG_FMT_I420:
    case IMG_FMT_I422:
    case IMG_FMT_I444:
        memset(m_pPlane[0], 0x10, m_iBytes[0]);
        memset(m_pPlane[1], 0x80, m_iBytes[1]);
        memset(m_pPlane[2], 0x80, m_iBytes[2]);
	break;
    case IMG_FMT_YUY2:
	for (unsigned i = 0; i < m_iBytes[0] / 4; i++)
	    avm_set_le32(((uint32_t*)m_pPlane[0]) + i, 0x80108010U);
        break;
    default: // RGB formats
        memset(m_pPlane[0], 0, m_iBytes[0]);
	break;
    }
}

void CImage::Convert(const uint8_t* from_data, const BitmapInfo* from_fmt)
{
    CImage ci(from_fmt, from_data);
    Convert(&ci);
}

void CImage::Convert(const CImage* from_img)
{
#if 0
    AVM_WRITE("CImage", "Convert from\n");
    from_img->GetFmt()->Print();
    const CImage* f = from_img;
    printf("From strides: %d, %d, %d  planes: %p, %p, %p\n", f->m_iStride[0], f->m_iStride[1], f->m_iStride[2], f->m_pPlane[0], f->m_pPlane[1], f->m_pPlane[2]);
    AVM_WRITE("CImage", "To  0x%x  -> 0x%x\n", Format(), from_img->Format());
    m_pInfo.Print();
#endif

    //assert(m_iHeight == from_img->m_iHeight
    //       && m_iWidth == from_img->m_iWidth);

    // FIXME
    bool flip_dir = ((GetFmt()->biHeight * from_img->GetFmt()->biHeight) < 0);

    //printf("FLIP %d\n", flip_dir);
    if (Format() == from_img->Format())
    {
	switch (Format())
	{
	case IMG_FMT_YV12:
	case IMG_FMT_I420:
	    yuvconv(this, from_img, flip_dir, I420TOYV12);
            return;
	case IMG_FMT_Y422:
	case IMG_FMT_UYVY:
	case IMG_FMT_YUY2:
	case 15:
	case 16:
	    lineconvert(this, from_img, flip_dir, BGR16TOBGR16);
	    return;
	case 24: lineconvert(this, from_img, flip_dir, BGR24TOBGR24); return;
	case 32: lineconvert(this, from_img, flip_dir, BGR32TOBGR32); return;
	default:
            printf("Format %.4s  0x%x\n", (const char*) &m_iFormat, m_iFormat);
	    //abort();
	}
    }

    if (from_img->Format() == 24)
    {
	anyFromRgb24(this, from_img, flip_dir);
	return;
    }
    switch (Format())
    {
    case 24:
	anyToRgb24(this, from_img, flip_dir);
	return;
    //shortcuts here
    case 16:
	switch (from_img->Format())
	{
	case 15: lineconvert(this, from_img, flip_dir, BGR15TOBGR16); return;
	case IMG_FMT_YV12:
	case IMG_FMT_I420:
	    yuvconv(this, from_img, flip_dir, YV12TOBGR16);
	    return;
	}
        break;
    case IMG_FMT_YUY2:
	switch (from_img->Format())
	{
	case IMG_FMT_YV12:
	case IMG_FMT_I420:
	    yuvconv(this, from_img, flip_dir, YV12TOYUY2);
	    return;
	}
        break;
    case IMG_FMT_YV12:
	switch (from_img->Format())
	{
	case IMG_FMT_YUY2:
	    yuvconv(this, from_img, flip_dir, YUY2TOYV12);
	    return;
	case IMG_FMT_Y422:
	case IMG_FMT_UYVY:
	    yuvconv(this, from_img, flip_dir, UYVYTOYV12);
	    return;
	case IMG_FMT_I420:
	case IMG_FMT_YV12: // copy
	    yuvconv(this, from_img, flip_dir, I420TOYV12);
            return;
	case IMG_FMT_I411:
	    yuvconv(this, from_img, flip_dir, I411TOYV12);
            return;
	case IMG_FMT_I422:
	    yuvconv(this, from_img, flip_dir, I422TOYV12);
            return;
	case IMG_FMT_I444:
	    yuvconv(this, from_img, flip_dir, I444TOYV12);
            return;
	case IMG_FMT_I410:
	    yuvconv(this, from_img, flip_dir, I410TOYV12);
            return;
	case IMG_FMT_Y800:
	    yuvconv(this, from_img, flip_dir, Y800TOYV12);
            return;
	}
        break;
    }

    CImage tmp(0, m_iWidth, m_iHeight);
    anyToRgb24(&tmp, from_img, flip_dir);
    anyFromRgb24(this, &tmp, false);
}

void CImage::Slice(const ci_surface_t* ci, int alpha)
{
    //printf("aaa %p %d %d %d   %d x %d   h:%d\n", planes, strides[0], strides[1], strides[2], w, h, y);
    //printf("bbb %p %d %d %d   %d x %d\n", m_pPlane, m_iStride[0], m_iStride[1], m_iStride[2], m_iWidth, m_iHeight);
    //printf("W %d   H %d  %d, %d   %d\n", m_Window.w, m_Window.h, m_Window.x, m_Window.y,  ci->m_Window.y);
    if (m_iFormat == ci->m_iFormat)
    {
	int w = ci->m_iWidth;
	if (m_Window.w < w)
            w = m_Window.w;
	int h = ci->m_iHeight;
	if (m_Window.h < h)
            h = m_Window.h;
	int d = 1;
        int dx = m_Window.x;
        int dy = m_Window.y;
	int cx = ci->m_Window.x;
	int cy = ci->m_Window.y;

	if (cx < dx)
	{
	    w -= (dx - dx);
	    cx = dx;
	}
	else
	    dx = cx;


	if (cy < dy)
	{
	    h -= (dy - cy);
	    cy = dy;
	}
	else
            dy = cy;

	//printf("WWW  %d   HHH %d  %d %d   %d %d \n", w, h, dx, cx, dy, cy);
	if (w > 0 && h > 0)
	{
	    switch (m_iFormat)
	    {
	    case IMG_FMT_YV12:
	    case IMG_FMT_I420:
		for (int i = 0; i < 3; i++)
		{
		    stride_memcpy(m_pPlane[i] + m_iStride[i] * dy / d + dx / d,
				  m_iStride[i],
				  ci->m_pPlane[i] + ci->m_iStride[i] * (cy - ci->m_Window.y) / d + (cx - ci->m_Window.x) / d,
				  ci->m_iStride[i],
				  w / d, h / d);
		    if (i == 0)
			d = 2;
		}
		emms();
		break;
	    }
	}
    }
}

void ci_surface_t::SetWindow(int x, int y, int w, int h)
{
    //printf("SET WINDOWS  %d  %d  %d  %d\n", x, y, w, h);
    m_Window.x = x; m_Window.y = y, m_Window.w = w; m_Window.h = h;
    if (w <= 0 || h <= 0 || x < 0 || y < 0
	|| x > m_iWidth || y > m_iWidth)
    {
	x = y = 0; m_Window.w = m_iWidth; m_Window.h = m_iHeight;
    }
    else
    {
	if ((x + w) > m_iWidth)
            m_Window.w = m_iWidth - x;
	if ((y + h) > m_iHeight)
	    m_Window.h = m_iHeight - y;
    }
    //printf("SET WINDOWS DONE  %d  %d  %d  %d\n", m_Window.x, m_Window.y, m_Window.w, m_Window.h);
}


uint8_t* ci_surface_t::GetWindow(uint_t idx)
{
    int dv = 1;
    //printf("GETWIN1   %d  %d\n", m_Window.x, m_iBpp);
    if (idx > 0)
    {
	switch (m_iFormat)
	{
	case IMG_FMT_YV12:
	case IMG_FMT_I420:
	    dv = 2;
	}
    }
    return m_pPlane[idx] + m_iStride[idx] * m_Window.y / dv + m_iBpp * m_Window.x / dv;
}

const uint8_t* ci_surface_t::GetWindow(uint_t idx) const
{
    int dv = 1;
    //printf("GETWIN2   %d  %d\n", m_Window.x, m_iBpp);
    if (idx > 0)
    {
	switch (m_iFormat)
	{
	case IMG_FMT_YV12:
	case IMG_FMT_I420:
	    dv = 2;
	}
    }

    return m_pPlane[idx] + m_iStride[idx] * m_Window.y / dv + m_iBpp * m_Window.x / dv;
}

AVM_END_NAMESPACE;
