#include "subtitle.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>

int subtitle_write(const subtitles_t* st, const char* filename, subtitle_t type)
{
    const char* nl = "\r\n";
    FILE* file = fopen(filename, "wt");

    if (!file)
        return -1;

    if (st->subtitle)
    {
	int i;
	for (i = 0; i < st->count; i++)
	{
	    int t1 = st->subtitle[i].start;
	    int t2 = st->subtitle[i].end;

	    int hh1 = (t1/60/60/1000);
	    int mm1 = (t1/60/1000)%60;
	    int ss1 = (t1/1000)%60;
	    int ms1 = (t1)%1000;
	    int hh2 = (t2/60/60/1000);
	    int mm2 = (t2/60/1000)%60;
	    int ss2 = (t2/1000)%60;
	    int ms2 = (t2)%1000;
            int tmp = 0;
	    int j;

	    switch (type)
	    {
	    case SUBTITLE_SUBRIP:
		fprintf(file, "%d%s%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d%s",
                        i + 1, nl, hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2, nl);
		for (j = 0; j < SUBTITLE_MAX_LINES; j++)
		    if (st->subtitle[i].line[j])
			fprintf(file, "%s%s", st->subtitle[i].line[j], nl);
		fputs(nl, file); /* extra new line */
		break;
	    case SUBTITLE_MICRODVD:
		fprintf(file, "{%d}{%d}",
			(int)(t1 * st->fps / 1000. + 0.5),
			(int)(t2 * st->fps / 1000. + 0.5));
		for (j = 0; j < SUBTITLE_MAX_LINES; j++)
		{
		    if (st->subtitle[i].line[j])
		    {
			if (tmp)
			    fputc('|', file);
			fprintf(file, "%s", st->subtitle[i].line[j]);
			tmp++;
		    }
		}
		fputs(nl, file);
		break;
	    default:
                return -1;
	    }
	}
    }

    return 0;
}

