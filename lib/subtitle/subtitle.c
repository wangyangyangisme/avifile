#include "subtitle.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static subtitle_line_t* copy_line(subtitle_line_t* to, const subtitle_line_t* from)
{
    int i;
    if (to)
    {
	for (i = 0; i < SUBTITLE_MAX_LINES; i++)
	{
	    if (from->line[i] && i < from->lines)
	    {
		unsigned l = strlen(from->line[i]);
		to->line[i] = (char*) realloc(to->line[i], l + 1);
		if (to->line[i])
		    strcpy(to->line[i], from->line[i]);
	    }
	    else if (to->line[i])
	    {
		free(to->line[i]);
		to->line[i] = NULL;
	    }
	}

	to->start = from->start;
	to->end = from->end;
	to->lines = from->lines;
    }
    return to;
}

static void free_line(subtitle_line_t* sl)
{
    int i;
    for (i = 0; i < sl->lines; i++)
    {
	free(sl->line[i]);
	sl->line[i] = NULL;
    }
    sl->start = 0;
    sl->end = 0;
    sl->lines = 0;
}

void subtitle_close(subtitles_t* st)
{
    if (st->subtitle)
    {
	int i;
	for (i = 0; i < st->count; i++)
	{
	    int j;
	    for (j = 0; j < SUBTITLE_MAX_LINES; j++)
	    {
		if (st->subtitle[i].line[j])
		    free(st->subtitle[i].line[j]);
	    }
	}
	free(st->subtitle);
    }
    if (st->encoding)
	free(st->encoding);
    free(st);
}

int subtitle_get(subtitle_line_t* sl, subtitles_t* st, double timepos)
{
    int lo = 0;
    int hi;
    unsigned int fp = (unsigned int) (timepos * 1000.0);
    subtitle_line_t* line = NULL;

    hi = st->count - 1;
    if (hi > 0)
    {
	if (st->frame_based && st->fps > 0.0)
	    fp = (unsigned int) (timepos * st->fps);

	//printf("get: %f -> %d  %f\n", timepos, fp, st->fps);
	while (lo < hi)
	{
	    int m = (lo + hi) / 2;

	    if (fp < st->subtitle[m].start)
		hi = m;
	    else if (fp >= st->subtitle[m + 1].start)
		lo = m + 1;
	    else
	    {
		lo = m;
		break;
	    }
	}
	if (st->subtitle[lo].start <= fp && fp < st->subtitle[lo].end)
	    line = &st->subtitle[lo];
    }
    //printf("pos %d    %p    %d   %d   %d\n", lo, line, fp, st->subtitle[lo].start, st->subtitle[lo].end);
    if (line)
    {
	if (!subtitle_line_equals(sl, line))
	    copy_line(sl, line);
    }
    else
	free_line(sl);

    return (!line) ? -1 : 0 ;
}

int subtitle_get_lines(subtitles_t* st)
{
    return st->count;
}

subtitle_t subtitle_get_type(subtitles_t* st, const char** r)
{
    static const char* t[] = {
	"unknown", "microdvd", "subrip", "vplayer", "aqt", "sami",
        "subviewer", "mpsub"
    };
    if (r && st->type < SUBTITLE_LAST)
	*r = t[st->type];
    return st->type;
}

subtitle_line_t* subtitle_line_new()
{
    subtitle_line_t* nl;
    nl = (subtitle_line_t*) malloc(sizeof(*nl));
    if (nl != NULL)
	memset(nl, 0, sizeof(*nl));
    return nl;
}

subtitle_line_t* subtitle_line_copy(const subtitle_line_t* sl)
{
    return (!sl) ? NULL : copy_line(subtitle_line_new(), sl);
}

int subtitle_line_equals(const subtitle_line_t* l1, const subtitle_line_t* l2)
{
    /*
     * for now we assume we have just one subtitle with the same time
     * and the line count
     */
    return (l1 != NULL && l2 != NULL
	    && l1->lines == l2->lines
	    && l1->start == l2->start
	    && l1->end == l2->end);
}

void subtitle_line_free(subtitle_line_t* sl)
{
    if (sl != NULL)
    {
        free_line(sl);
	free(sl);
    }
}

void subtitle_set_fps(subtitles_t* st, double fps)
{
    st->fps = fps;
}
