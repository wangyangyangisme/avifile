/*
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 2-pass code OpenDivX port:
 * Copyright (C) 2001 Christoph Lampert <gruel@gmx.de>
 *
 * transcode port:
 * Copyright (C) Thomas Östreich - June 2001
 *
 * avifile port:
 * Copyright (C) Zdenek Kabelac - July 2002 <kabi@users.sf.net>
 *
 */

/*
 * Two-pass-code from OpenDivX
 *
 * Large parts of this code were taken from VbrControl()
 * from the OpenDivX project, (C) divxnetworks,
 * this code is published under DivX Open license, which
 * can be found... somewhere... oh, whatever...
 */

/*
 * I don't like it at all - ok avifile is C++ project originaly
 * but there seems to be a big aversion in linux comunity for C++
 * ok so let's write C code in C++ way - this should have make
 * everyone happy...
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <inttypes.h>

#include "vbrctrl.h"

#define FALSE 0
#define TRUE 1

/*  Absolute maximum and minimum quantizers used in VBR modes */
static const int MIN_QUANTIZER = 1;
static const int MAX_QUANTIZER = 31;

/*  Limits on frame-level deviation of quantizer ( higher values
 *  correspond to frames with more changes and vice versa ) */
static const float MIN_QUANT_DELTA = -10.;
static const float MAX_QUANT_DELTA = 5.;

/*  Limits on stream-level deviation of quantizer ( used to make
 *  overall bitrate of stream close to requested value ) */
static const float MIN_RC_QUANT_DELTA = .6;
static const float MAX_RC_QUANT_DELTA = 1.5;

/*  Crispness parameter	controls threshold for decision whether
 *  to skip the frame or to code it. */
//static const float MAX_CRISPENSS=100.f;
/*	Maximum allowed number of skipped frames in a line. */
//static const int MAX_DROP_LINE=0;
// CHL   We don't drop frames at the moment!

// methods from class VbrControl
typedef struct {
    /* max 28 bytes/frame or 5 Mb for 2-hour movie */
    int quant;
    int text_bits;
    int motion_bits;
    int total_bits;
    float mult;
    short is_key_frame;
    short drop;
} vbrentry_t;

struct vbrctrl_s {
    int m_iCount;
    int m_iQuant;
    int m_iCrispness;
    short m_bDrop;
    float m_fQuant;

    int64_t m_lEncodedBits;
    int64_t m_lExpectedBits;

    FILE *m_pFile;

    int m_iEntries;
    vbrentry_t *m_pFrames;

    int iNumFrames;
};

vbrctrl_t* vbrctrl_init_1pass(int quality, int crispness)
{
    vbrctrl_t* t = malloc(sizeof(vbrctrl_t));

    if (t) {
	memset(t, 0, sizeof(vbrctrl_t));
	t->m_fQuant = MIN_QUANTIZER + ((MAX_QUANTIZER - MIN_QUANTIZER) / 6.) * (6 - quality);
	t->m_iCount = 0;
	t->m_bDrop = FALSE;
	vbrctrl_update_1pass(t);
    }
    return t;
}

vbrctrl_t* vbrctrl_init_2pass_analysis(const char *filename,
				       int quality, int crispness)
{
    vbrctrl_t* t;
    FILE *f = fopen(filename, "wb");

    if (!f == 0)
	return 0;

    t = vbrctrl_init_1pass(quality, crispness);
    if (!t) {
	fclose(f);
	return 0;
    }
    t->m_iCount = 0;
    t->m_bDrop = FALSE;
    t->m_pFile = f;
    fprintf(f, "##version 1\n");
    fprintf(f, "quality %d\n", quality);
    return t;
}

vbrctrl_t* vbrctrl_init_2pass_encoding(const char *filename, int bitrate,
				       double framerate, int quality,
				       int crispness)
{
    int i;
    vbrctrl_t* t;
    int64_t text_bits = 0;
    int64_t total_bits = 0;
    int64_t complexity = 0;
    int64_t new_complexity = 0;
    int64_t motion_bits = 0;
    int64_t denominator = 0;
    float qual_multiplier = 1.;
    char head[20];

    int64_t desired_bits;
    int64_t non_text_bits;

    float average_complexity;

    FILE *f = fopen(filename, "rb");

    if (!f)
	return 0;

    t = vbrctrl_init_1pass(quality, 0);
    if (!t) {
	fclose(f);
	return 0;
    }

    t->m_bDrop = FALSE;
    t->m_iCount = 0;

    fread(head, 10, 1, f);
    if (!strncmp("##version ", head, 10)) {
	int version;
	int iOldQual;
	float old_qual, new_qual;

	fscanf(f, "%d\n", &version);
	fscanf(f, "quality %d\n", &iOldQual);
	switch (iOldQual) {
	case 5:
	    old_qual = 1.;
	    break;
	case 4:
	    old_qual = 1.1;
	    break;
	case 3:
	    old_qual = 1.25;
	    break;
	case 2:
	    old_qual = 1.4;
	    break;
	case 1:
	default:
	    old_qual = 2.;
	    break;
	}
	switch (quality) {
	case 5:
	    new_qual = 1.;
	    break;
	case 4:
	    new_qual = 1.1;
	    break;
	case 3:
	    new_qual = 1.25;
	    break;
	case 2:
	    new_qual = 1.4;
	    break;
	case 1:
	default:
	    new_qual = 2.;
	    break;
	}
	qual_multiplier = new_qual / old_qual;
    } else
	fseek(f, 0, SEEK_SET);

    /* removed C++ dependencies, now read file twice :-( */

    t->iNumFrames = 0;
    while (!feof(f)) {
	vbrentry_t *e;

	if (t->m_iEntries < (t->iNumFrames + 1)) {
	    t->m_iEntries += 20000;	// realloc for every 20000 frames;
	    t->m_pFrames = realloc(t->m_pFrames, t->m_iEntries * sizeof(vbrentry_t));
	    if (!t->m_pFrames)
	    {
		fclose(f);
                vbrctrl_close(t);
		return 0; // out of memory
	    }
	}
	e = &(t->m_pFrames[t->iNumFrames]);
	fscanf(f, "Frame %*d: intra %hd, quant %d, texture %d, motion %d, total %d\n",
	       &(e->is_key_frame), &(e->quant), &(e->text_bits), &(e->motion_bits),
	       &(e->total_bits));

	e->total_bits += e->text_bits * (qual_multiplier - 1);
	e->text_bits *= qual_multiplier;
	text_bits += e->text_bits;
	motion_bits += e->motion_bits;
	total_bits += e->total_bits;
	complexity += e->text_bits * e->quant;
	t->iNumFrames++;
	//      printf("Frames %d, texture %d, motion %d, quant %d total %d ",
	//              iNumFrames, vFrame.text_bits, vFrame.motion_bits, vFrame.quant, vFrame.total_bits);
	//      printf("texture %d, total %d, complexity %lld \n",vFrame.text_bits,vFrame.total_bits, complexity);
    }
    fclose(f);

    desired_bits = (int64_t) bitrate *(int64_t) t->iNumFrames / framerate;
    non_text_bits = total_bits - text_bits;

    //if (verbose & TC_DEBUG) {
    //    fprintf(stderr, "(%s) frames %d, texture %lld, motion %lld, total %lld, complexity %lld\n", __FILE__, iNumFrames, text_bits, motion_bits, total_bits, complexity);
    //}

    if (desired_bits <= non_text_bits) {
	char s[200];

	printf("Specified bitrate is too low for this clip.\n"
	       "Minimum possible bitrate for the clip is %.0f kbps. Overriding\n"
	       "user-specified value.\n",
	       (float) (non_text_bits * framerate / (int64_t) t->iNumFrames));

	desired_bits = non_text_bits * 3 / 2;
	/*
	   m_fQuant=MAX_QUANTIZER;
	   for(int i=0; i<iNumFrames; i++)
	   {
	   m_pFrames[i].drop=0;
	   m_pFrames[i].mult=1;
	   }
	   vbrctrl_set_quant(m_fQuant);
	   return 0;
	 */
    }

    desired_bits -= non_text_bits;
    /**
     BRIEF EXPLANATION OF WHAT'S GOING ON HERE.
     We assume that
     text_bits=complexity / quantizer
     total_bits-text_bits = const(complexity)
     where 'complexity' is a characteristic of the frame
     and does not depend much on quantizer dynamics.
     Using this equation, we calculate 'average' quantizer
     to be used for encoding ( 1st order effect ).
     Having constant quantizer for the entire stream is not
     very convenient - reconstruction errors are
     more noticeable in low-motion scenes. To compensate
     this effect, we multiply quantizer for each frame by
     (complexity/average_complexity)^k,
     ( k - parameter of adjustment ). k=0 means 'no compensation'
     and k=1 is 'constant bitrate mode'. We choose something in
     between, like 0.5 ( 2nd order effect ).
     **/

    average_complexity = complexity / t->iNumFrames;

    for (i = 0; i < t->iNumFrames; i++) {
	float mult;

	if (t->m_pFrames[i].is_key_frame) {
	    if ((i + 1 < t->iNumFrames) && (t->m_pFrames[i + 1].is_key_frame))
		mult = 1.25;
	    else
		mult = .75;
	} else {
	    mult = t->m_pFrames[i].text_bits * t->m_pFrames[i].quant;
	    mult = (float) sqrt(mult / average_complexity);

	    //if(i && m_pFrames[i-1].is_key_frame)
	    //    mult *= 0.75;
	    if (mult < 0.5)
		mult = 0.5;
	    if (mult > 1.5)
		mult = 1.5;
	}

	t->m_pFrames[i].mult = mult;
	t->m_pFrames[i].drop = FALSE;
	new_complexity += t->m_pFrames[i].text_bits * t->m_pFrames[i].quant;
	denominator += desired_bits * t->m_pFrames[i].mult / t->iNumFrames;
    }

    t->m_fQuant = ((double) new_complexity) / (double) denominator;

    if (t->m_fQuant < MIN_QUANTIZER)
	t->m_fQuant = MIN_QUANTIZER;
    if (t->m_fQuant > MAX_QUANTIZER)
	t->m_fQuant = MAX_QUANTIZER;
    t->m_pFile = fopen("analyse.log", "wb");
    if (t->m_pFile) {
	fprintf(t->m_pFile, "Total frames: %d Avg quantizer: %f\n", t->iNumFrames, t->m_fQuant);
	fprintf(t->m_pFile, "Expecting %12lld bits\n", desired_bits + non_text_bits);
	fflush(t->m_pFile);
    }
    vbrctrl_set_quant(t, t->m_fQuant * t->m_pFrames[0].mult);
    t->m_lEncodedBits = t->m_lExpectedBits = 0;
    return t;
}

int vbrctrl_get_intra(vbrctrl_t* t)
{
    return t->m_pFrames[t->m_iCount].is_key_frame;
}

int vbrctrl_get_drop(vbrctrl_t* t)
{
    return t->m_bDrop;
}

int vbrctrl_get_quant(vbrctrl_t* t)
{
    return t->m_iQuant;
}

void vbrctrl_set_quant(vbrctrl_t* t, float quant)
{
    t->m_iQuant = quant;
    if ((rand() % 10) < ((quant - t->m_iQuant) * 10))
	t->m_iQuant++;
    if (t->m_iQuant < MIN_QUANTIZER)
	t->m_iQuant = MIN_QUANTIZER;
    else if (t->m_iQuant > MAX_QUANTIZER)
	t->m_iQuant = MAX_QUANTIZER;
}

void vbrctrl_update_1pass(vbrctrl_t* t)
{
    vbrctrl_set_quant(t, t->m_fQuant);
    t->m_iCount++;
}

void vbrctrl_update_2pass_analysis(vbrctrl_t* t, int is_key_frame, int motion_bits,
				   int texture_bits, int total_bits, int quant)
{
    if (t->m_pFile)
    {
	fprintf(t->m_pFile, "Frame %d: intra %d, quant %d, texture %d, motion %d, total %d\n",
		t->m_iCount, is_key_frame, quant, texture_bits, motion_bits, total_bits);
	t->m_iCount++;
    }
}

void vbrctrl_update_2pass_encoding(vbrctrl_t* t, int motion_bits,
				   int texture_bits, int total_bits)
{
    float q;
    double dq;
    vbrentry_t *e;

    if (t->m_iCount >= t->iNumFrames)
	return;
    e = &(t->m_pFrames[t->m_iCount]);

    t->m_lExpectedBits += (e->total_bits - e->text_bits) + e->text_bits * e->quant / t->m_fQuant;
    t->m_lEncodedBits += (int64_t) total_bits;

    if (t->m_pFile) {
	fprintf(t->m_pFile,
		"Frame %d: PRESENT, complexity %d, quant multiplier %f, texture %d, total %d ",
		t->m_iCount, e->text_bits * e->quant, e->mult, texture_bits, total_bits);
    }

    t->m_iCount++;

    q = t->m_fQuant * t->m_pFrames[t->m_iCount].mult;
    if (q < t->m_fQuant + MIN_QUANT_DELTA)
	q = t->m_fQuant + MIN_QUANT_DELTA;
    if (q > t->m_fQuant + MAX_QUANT_DELTA)
	q = t->m_fQuant + MAX_QUANT_DELTA;

    dq = (float) t->m_lEncodedBits / (float) t->m_lExpectedBits;
    dq *= dq;
    if (dq < MIN_RC_QUANT_DELTA)
	dq = MIN_RC_QUANT_DELTA;
    if (dq > MAX_RC_QUANT_DELTA)
	dq = MAX_RC_QUANT_DELTA;
    if (t->m_iCount < 20)	// no framerate corrections in first frames
	dq = 1.;
    q *= dq;
    vbrctrl_set_quant(t, q);
    if (t->m_pFile)
	fprintf(t->m_pFile, "Progress: expected %12lld, achieved %12lld, "
		"dq %f, new quant %d\n", t->m_lExpectedBits,
		t->m_lEncodedBits, dq, t->m_iQuant);
}

void vbrctrl_close(vbrctrl_t* t)
{
    if (t) {
	if (t->m_pFile)
	    fclose(t->m_pFile);
	if (t->m_pFrames)
	    free(t->m_pFrames);
	free(t);
    }
}
