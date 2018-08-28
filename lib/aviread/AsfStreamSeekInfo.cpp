#include "AsfInputStream.h"

//#include <stdio.h>

/**
 *
 * This object contains information that's necessary for proper implementation
 * of seek and a few other nice features. To create it, we need to read the
 * whole media first. That's why we won't do it for non-local media.
 */

AVM_BEGIN_NAMESPACE;

framepos_t AsfStreamSeekInfo::prevKeyFrame(framepos_t kf) const
{
    if (kf == 0 || kf >= size() || kf == ERR)
	return ERR;

    for (uint_t i = kf - 1; i > 0; i--)
	if (operator[](i).IsKeyFrame())
	    return i;

    return 0;
}

framepos_t AsfStreamSeekInfo::nextKeyFrame(framepos_t kf) const
{
    if (kf >= size() || kf == ERR)
	return ERR;

    for (uint_t i = kf + 1; i < size(); i++)
	if (operator[](i).IsKeyFrame())
	    return i;

    return ERR;
}

framepos_t AsfStreamSeekInfo::nearestKeyFrame(framepos_t kf) const
{
    if (kf >= size() || kf == ERR)
	return ERR;

    framepos_t prev_kf = prevKeyFrame(kf);
    framepos_t next_kf = nextKeyFrame(kf);

    return (kf - prev_kf < next_kf - kf) ? prev_kf : next_kf;
}

framepos_t AsfStreamSeekInfo::find(uint_t sktime) const
{
    if (!size() || sktime == ERR)
	return ERR;

    framepos_t h = size() - 1;
    framepos_t l = (sktime < operator[](h).object_start_time) ? 0 : h;

    while (l != h)
    {
	framepos_t m = (l + h) / 2;
	if (sktime >= operator[](m).object_start_time)
	{
	    if (l == m)
		break;

	    l = m;
	    if (sktime < operator[](m + 1).object_start_time)
                break;
	}
	else
	    h = m;
    }
#if 0
    printf("TIME find sk: %f   cur: %f  nex: %f  -> %d\n",
	   sktime/1000.0,  operator[](l).object_start_time/1000.0,
	   operator[](l + 1).object_start_time/1000.0, l);
#endif
    return l;
}

AVM_END_NAMESPACE;
