/* $Id: deinterlace-rgb.cpp,v 1.6 2004/11/12 12:15:31 kabi Exp $ */

#include <config.h>
#include <string.h>

/* copied from qtrenderer.c in avifile package samples/qtvidcap */

void copy_deinterlace_24(void* outpic, const void* inpic, int xdim, int height)
{
#ifdef ARCH_X86
    for(int i=0; i<height; i++)
    {
	char* outp=((char*)outpic)+i*xdim;
	const char* inp=((const char*)inpic)+i*xdim;
	if((i==0) || (i==height-1))
	{
	    memcpy(outp, inp, xdim);
	    continue;
	}
	int count=xdim/8;
	__asm__ __volatile__ (
	"movl %2, %%esi\n\t"
	"movl %3, %%edi\n\t"	
	"pxor %%mm3, %%mm3\n\t"
	"1:\n\t"
	"movq (%%ecx, %%esi), %%mm0\n\t"
	"movq (%%ecx), %%mm1\n\t"
	"movq %%mm1, %%mm2\n\t"
	"pavgb %%mm0, %%mm2\n\t"
	"psadbw %%mm2, %%mm3\n\t"
	"movd %%mm3, %%eax\n\t"
	"cmpl $48, %%eax\n\t"
	"jb 2f\n\t"
	"pavgb %%mm1, %%mm0\n\t"
	"2:\n\t"
	"movq %%mm0, (%%edx)\n\t"
	"addl $8, %%ecx\n\t"
	"addl $8, %%edx\n\t"
	"decl %%edi\n\t"
	"jnz 1b\n\t"
    	:
	: "c" (inp-xdim), "d" (outp), "r" (xdim), "r" (count)
	: "esi", "edi"
	);
    }
    __asm__ __volatile__ ("emms\n\t");
#else
#warning missing copy_deinterlace_24
#endif
}
