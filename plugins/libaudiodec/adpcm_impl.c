/***********************************************************
Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/*
** Intel/DVI ADPCM coder/decoder.
**
** The algorithm for this coder was taken from the IMA Compatability Project
** proceedings, Vol 2, Number 2; May 1992.
**
** Version 1.2, 18-Dec-92.
**
** Change log:
** - Decoder finaly fixed by kabi@users.sourceforge.net 20011204
** - Some major modification by kabi@users.sourceforge.net
** - Fixed a stupid bug, where the delta was computed as
**   stepsize*code/4 in stead of stepsize*(code+0.5)/4.
** - There was an off-by-one error causing it to pick
**   an incorrect delta once in a blue moon.
** - The NODIVMUL define has been removed. Computations are now always done
**   using shifts, adds and subtracts. It turned out that, because the standard
**   is defined using shift/add/subtract, you needed bits of fixup code
**   (because the div/mul simulation using shift/add/sub made some rounding
**   errors that real div/mul don't make) and all together the resultant code
**   ran slower than just using the shifts all the time.
** - Changed some of the variable names to be more meaningful.
*/

#include "adpcm_impl.h"
#if 0
#include <stdio.h> /*DBG*/
#endif

#ifndef __STDC__
#define signed
#endif

/* Intel ADPCM step variation table */
/* 4bit DVI ADPCM */
static const int index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

/* 3bit is different */
/*
 *
 * Lookup tables for IMA ADPCM format
 *
 */
#define ISSTMAX 88

static const int stepsize_table[ISSTMAX + 1] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

/* +0 - +3, decrease step size */
/* +4 - +7, increase step size */
/* -0 - -3, decrease step size */
/* -4 - -7, increase step size */
static int tabinit = 0;
static unsigned char index_adjust_table[ISSTMAX+1][8];

void adpcm_init_table(void)
{
    if (!tabinit)
    {
	int i, j, k;
	for (i = 0; i <= ISSTMAX; i++)
	{
	    for (j = 0; j < 8; j++)
	    {
		k = i + (j < 4) ? -1: (2* j -6);
		if (k < 0)
		    k = 0;
		else if (k > ISSTMAX)
		    k = ISSTMAX;
		index_adjust_table[i][j] = k;
	    }
	}
        tabinit = 1;
    }
}

void
adpcm_coder(unsigned char* outdata, const short* indata, unsigned int len,
	    struct adpcm_state* state)
{
    int val;			/* Current input sample value */
    int sign;			/* Current adpcm sign bit */
    int delta;			/* Current adpcm output value */
    int diff;			/* Difference between val and valprev */
    int step;			/* Stepsize */
    int valpred;		/* Predicted output value */
    int vpdiff;			/* Current change to valpred */
    int index;			/* Current step change index */
    int outputbuffer = 0;      	/* place to keep previous 4-bit value */
    int bufferstep = 1;		/* toggle between outputbuffer/output */

    valpred = state->valprev;
    index = state->index;
    step = stepsize_table[index];

    for ( ; len > 0 ; len-- ) {
	val = *indata++;

	/* Step 1 - compute difference with previous value */
	diff = val - valpred;
	sign = (diff < 0) ? 8 : 0;
	if ( sign ) diff = (-diff);

	/* Step 2 - Divide and clamp */
	/* Note:
	** This code *approximately* computes:
	**    delta = diff*4/step;
	**    vpdiff = (delta+0.5)*step/4;
	** but in shift step bits are dropped. The net result of this is
	** that even if you have fast mul/div hardware you cannot put it to
	** good use since the fixup would be too expensive.
	*/
	vpdiff = (step >> 3);

	if ( diff >= step ) {
	    delta = 4;
	    diff -= step;
	    vpdiff += step;
	}
	else
	    delta = 0;

	step >>= 1;
	if ( diff >= step ) {
	    delta |= 2;
	    diff -= step;
	    vpdiff += step;
	}
	step >>= 1;
	if ( diff >= step ) {
	    delta |= 1;
	    vpdiff += step;
	}

	/* Step 3 - Update previous value */
	if ( sign )
	  valpred -= vpdiff;
	else
	  valpred += vpdiff;

	/* Step 4 - Clamp previous value to 16 bits */
	if ( valpred > 32767 )
	  valpred = 32767;
	else if ( valpred < -32768 )
	  valpred = -32768;

	/* Step 5 - Assemble value, update index and step values */
	delta |= sign;

	index += index_table[delta];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;
	step = stepsize_table[index];

	/* Step 6 - Output value */
	if ( bufferstep ) {
	    outputbuffer = (delta << 4) & 0xf0;
	} else {
	    *outdata++ = (delta & 0x0f) | outputbuffer;
	}
	bufferstep = !bufferstep;
    }

    /* Output last step, if needed */
    if ( !bufferstep )
      *outdata++ = outputbuffer;

    state->valprev = valpred;
    state->index = index;
}

void
adpcm_decoder(short* outdata, const void* indata, unsigned int len,
	      struct adpcm_state* state, unsigned int channels)
{
    const unsigned char* inp = (const unsigned char *)indata; /* Input buffer pointer */
    int valpred;		/* Predicted value */
    int index;			/* Current step change index */
    int i;

    valpred = state->valprev;
    index = state->index;

    inp -= 4 * (channels - 1); // compensate first add
    for (i = 0; i < len; i++) {
	int delta;			/* Current adpcm output value */
	int step;			/* Stepsize */
	int vpdiff;			/* Current change to valpred */

	/* Step 1 - get the delta value */
	if (i & 1) {
	    delta = (*inp++ >> 4) & 0x0f;
	} else {
	    if (!(i & 7))
		inp += 4 * channels - 4;
	    delta = *inp & 0x0f;
	}

	/* Step 1.5 - Update step value */
	step = stepsize_table[index];

	/* Step 2 - Find new index value (for later) */
	index += index_table[delta];
	if ( index < 0 ) index = 0;
	else if ( index > 88 ) index = 88;


	/* Step 4 - Compute difference and new predicted value */
	/*
	** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
	** in adpcm_coder.
	*/
#if 0
	vpdiff = step >> 3;
	if ( delta & 4 ) vpdiff += step;
	if ( delta & 2 ) vpdiff += step >> 1;
	if ( delta & 1 ) vpdiff += step >> 2;
#else
	/** but we will use (2*delta + 1) * step / 8  */
	vpdiff = ((2 * (delta & 7) + 1) * step) >> 3;
#endif
	/* Step 3 - Separate sign and magnitude */
	if ( delta & 8 )
	{
	    valpred -= vpdiff;
	    if (valpred < -32768)
		valpred = -32768;
	}
	else
	{
	    valpred += vpdiff;
	    if (valpred > 32767)
		valpred = 32767;
	}

	/* Step 7 - Output value */
	*outdata = valpred;
        outdata += channels;
    }

    state->valprev = valpred;
    state->index = index;
}
