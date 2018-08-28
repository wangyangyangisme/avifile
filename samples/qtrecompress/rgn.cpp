#include "rgn.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

template<class T> T mymin(const T x, const T y) {return (x<y)?x:y;}

void CEdgeRgn::Maximize(unsigned char* data, int xd, int yd, int depth)
{

#ifdef ARCH_X86
    const int limit=xd*yd;
    int64_t t2 = 0x8080808080808080LL;
    unsigned  lim=1<<depth;
    unsigned  ach=1;
    unsigned  char* p=(unsigned  char*)data;

    char g_pstore[200];
    __asm__ __volatile__
    ("fsave (%0)\n\t"
    "emms\n\t"
    :
    :"r"(&g_pstore));

    while (ach < lim)
    {
    	__asm__ __volatile__ (
	//p %0
    	//ach %1
	//limit %2
		"	pushl %%eax\n\t"
			"pushl %%ebx\n\t"
			"pushl %%ecx\n\t"
			"pushl %%edx\n\t"
			"pushl %%edi\n\t"

			"movl %0, %%ebx\n\t"

			"movl %1, %%ecx\n\t"

			"movl %0, %%edx\n\t"
			"addl %2, %%edx\n\t"
			"subl %1, %%edx\n\t"

			"movl %%edx, %%edi\n\t"

			"movq %3, %%mm2\n\t"

			"xorl %%eax, %%eax\n\t"
			"xorl %%edx, %%edx\n\t"
"1:\n\t"
			"movq (%%ebx), %%mm0\n\t"
			"movq (%%ebx,%%ecx), %%mm1\n\t"
			"pxor %%mm2, %%mm0\n\t"
			"pxor %%mm2, %%mm1\n\t"
			"movq %%mm0, %%mm3\n\t"
			"pcmpgtb %%mm1, %%mm3\n\t"
			"pand %%mm3, %%mm0\n\t"
			"pandn %%mm1, %%mm3\n\t"
			"por %%mm3, %%mm0\n\t"
			"pxor %%mm2, %%mm0\n\t"
			"movq %%mm0, (%%ebx)\n\t"

			"add $8, %%ebx\n\t"
			"cmp %%edi, %%ebx\n\t"
			"jb 1b\n\t"

			"popl %%edi\n\t"
			"popl %%edx\n\t"
			"popl %%ecx\n\t"
			"popl %%ebx\n\t"
			"popl %%eax\n\t"
		:
		:"r"(p), "r"(ach), "r"(limit), "m"(t2)
		);

		ach*=xd;

		__asm__ __volatile__ (
		//p %0
		//ach %1
		//limit %2
		"	pushl %%eax\n\t"
			"pushl %%ebx\n\t"
			"pushl %%ecx\n\t"
			"pushl %%edx\n\t"
			"pushl %%edi\n\t"

			"movl %0, %%ebx\n\t"

//			"movl %0, %%ecx\n\t"
//			"addl %1, %%ecx\n\t"

			"movl %0, %%ebx\n\t"

			"movl %1, %%ecx\n\t"

			"movl %0, %%edx\n\t"
			"addl %2, %%edx\n\t"
			"subl %1, %%edx\n\t"

			"movl %%edx, %%edi\n\t"

			"movq %3, %%mm2\n\t"


			"xorl %%eax, %%eax\n\t"
			"xorl %%edx, %%edx\n\t"
//			align 8
"2:\n\t"
			"movq (%%ebx), %%mm0\n\t"
			"movq (%%ebx,%%ecx), %%mm1\n\t"
			"pxor %%mm2, %%mm0\n\t"
			"pxor %%mm2, %%mm1\n\t"
			"movq %%mm0, %%mm3\n\t"
			"pcmpgtb %%mm1, %%mm3\n\t"
			"pand %%mm3, %%mm0\n\t"
			"pandn %%mm1, %%mm3\n\t"
			"por %%mm3, %%mm0\n\t"
			"pxor %%mm2, %%mm0\n\t"
			"movq %%mm0, (%%ebx)\n\t"

			"add $8, %%ebx\n\t"
			"cmp %%edi, %%ebx\n\t"
			"jb 2b\n\t"

			"popl %%edi\n\t"
			"popl %%edx\n\t"
			"popl %%ecx\n\t"
			"popl %%ebx\n\t"
			"popl %%eax\n\t"
		:
		:"r"(p), "r"(ach), "r"(limit), "m"(t2)
		);
		ach/=xd;
		ach*=2;
	}
	__asm__ __volatile__ ("frstor (%0)\n\t": :"r"(&g_pstore));
    return;
#else
#warning MAXIMIZE is only i386 - C version missing
#endif
}

CEdgeRgn::CEdgeRgn(avm::CImage* im, int _qual, int edge, bool, int zz)
    :avm::CImage(new avm::BitmapInfo(im->Width(), im->Height(), 8))
{
    if(im->GetFmt()->IsRGB())
    {
	fprintf(stderr, "Edge detection with non-YUV image\n");
	return;
    }
    if (zz == 0)
	zz = 1;
    if (edge== 0)
	edge = 1;
    const int avs = ((1<<zz)+(1<<edge))/2-1;

    unsigned char* res = new unsigned char[Pixels()];

    unsigned char *data = Data();
    memset(data, 255, Pixels());
    int i;
    int max;
    int upl = Pixels();

    avm::CImage* src = new avm::CImage(im);

    src->Blur(edge);
    for (i = 0; i < upl; i++)
	res[i]=((avm::yuv*)src->Data())[i].Y;

    Maximize(res, Width(), Height(), zz);

    memset(data, 0, upl);
    //	for(i=0; i<upl; i++)
    //		_data[i]=res[i];
    //		_data[i]=128+2*(res[i]-((struct yuv*)src->data())[i].Y);

    upl -= (Width() * avs + avs);
    int dist = avs * (Width() + 1);
    for (i = avs + Width() * avs; i < upl; i++)
    {
	max = res[i-dist];
	int val = max - ((avm::yuv*)src->Data())[i].Y;
	data[i] = (val < (_qual-256))
	    ? 255 : 128 - mymin((val-_qual + 256) * 2, 128);
    }

    delete src;
    delete res;
}
void CEdgeRgn::Normalize()
{
    for (unsigned char* ch = Data() + Pixels() - 1;
	 ch >= Data(); ch--)
	*ch = (*ch > 128) ? 0 : 255;
}

void CEdgeRgn::Blur(int depth, int src)
{
#ifdef ARCH_X86
    const int limit = Pixels();
    unsigned lim = 1 << depth;
    unsigned ach = 1 << src;
    unsigned char* p = Data();

    while (ach < lim)
    {
	__asm__ __volatile__ (
			      "pushl %%eax\n\t"
			      "pushl %%ebx\n\t"
			      "pushl %%ecx\n\t"
			      "pushl %%edx\n\t"
			      "pushl %%edi\n\t"
			      "movl %0, %%ebx\n\t"

			      "mov %0, %%ecx\n\t"
			      "add %1, %%ecx\n\t"

			      "mov %0, %%ebx\n\t"

			      "mov %1, %%ecx\n\t"

			      "mov %0, %%edx\n\t"
			      "add %2, %%edx\n\t"
			      "sub %1, %%edx\n\t"

			      "movl %%edx, %%edi\n\t"

			      // %%ebx = p
			      // %%ecx = ach
			      // %%edi = p + limit - ach

			      "xor %%eax, %%eax\n\t"
			      "xor %%edx, %%edx\n\t"
			      "0:\n\t"
			      "movb (%%ebx), %%al\n\t"
			      "movb (%%ebx, %%ecx), %%dl\n\t"
			      "addw %%dx, %%ax\n\t"
			      "shr $1, %%eax\n\t"
			      "movb %%al, (%%ebx)\n\t"
			      "incl %%ebx\n\t"
			      "cmpl %%edi, %%ebx\n\t"
			      "jb 0b\n\t"

			      "popl %%edi\n\t"
			      "popl %%edx\n\t"
			      "popl %%ecx\n\t"
			      "popl %%ebx\n\t"
			      "popl %%eax\n\t"
			      :
			      :"r"(p), "r"(ach), "r"(limit)
			     );


	ach *= Width();

	__asm__ __volatile__ (
			      "pushl %%eax\n\t"
			      "pushl %%ebx\n\t"
			      "pushl %%ecx\n\t"
			      "pushl %%edx\n\t"
			      "pushl %%edi\n\t"
			      "movl %0, %%ebx\n\t"

			      "mov %0, %%ecx\n\t"
			      "add %1, %%ecx\n\t"

			      "mov %0, %%ebx\n\t"

			      "mov %1, %%ecx\n\t"

			      "mov %0, %%edx\n\t"
			      "add %2, %%edx\n\t"
			      "sub %1, %%edx\n\t"

			      "movl %%edx, %%edi\n\t"

			      // %%ebx = p
			      // %%ecx = ach
			      // %%edi = p + limit - ach

			      "xor %%eax, %%eax\n\t"
			      "xor %%edx, %%edx\n\t"
			      "1:\n\t"
			      "movb (%%ebx), %%al\n\t"
			      "movb (%%ebx, %%ecx), %%dl\n\t"
			      "addw %%dx, %%ax\n\t"
			      "shr $1, %%eax\n\t"
			      "movb %%al, (%%ebx)\n\t"
			      "incl %%ebx\n\t"
			      "cmpl %%edi, %%ebx\n\t"
			      "jb 1b\n\t"

			      "popl %%edi\n\t"
			      "popl %%edx\n\t"
			      "popl %%ecx\n\t"
			      "popl %%ebx\n\t"
			      "popl %%eax\n\t"
			      :
			      :"r"(p), "r"(ach), "r"(limit)
			     );

	ach /= Width();
	ach*=2;
    }
#else
#warning Blur x86 version only - FIXME
#endif
}
