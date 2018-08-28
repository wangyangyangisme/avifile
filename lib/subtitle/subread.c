#include "config.h"
#include "subtitle.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <langinfo.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

static void skip_to_eol(FILE* file)
{
    int c;

    while ((c = fgetc(file)))
	if (c != '\r' || c != '\n')
	    break;
    ungetc(c, file);
}

static inline char* skip_spaces(char* buf)
{
    while (*buf && isspace((int)*buf))
	buf++;

    return buf;
}

static inline char* trim_spaces(char* buf)
{
    char* e = buf = skip_spaces(buf);
    while (*e)
	e++;

    while (e > buf && isspace((int)e[-1]))
	e--;
    *e = 0;
    return buf;
}

static inline void trim_http(char* buf)
{
    enum { normal, http } state = normal;
    char* e = buf;
    char* d = e;

    while (*e) {
	if (*e == '<' && (toupper(e[1]) == 'I' || e[1] == '/'))
	    state = http;
	else if (state == http)
	{
	    if (*e == '>')
		state = normal;
	}
	else
	    *d++ = *e;
        e++;
    }
    *d = 0;
}

static inline unsigned int get_subtime(int sh, int sm, int ss, int su)
{
    return ((sh * 60 + sm) * 60 + ss) * 1000 + su;
}

// read line and remove window \r character from it
static char* read_line(char* buf, size_t size, FILE* file)
{
    char *endb = buf + size - 1;
    char *beg = buf;

    while (beg < endb)
    {
	int c = fgetc(file);
	if (c == EOF)
	{
	    if (beg != buf)
		break;

	    return 0;
	}
	if (c == '\n')
	    break;    // we have line
	if (c != '\r')
	    *beg++ = c;
    }
    *beg = 0;
    return buf;
}

static void add_line(subtitles_t* l, subtitle_line_t* sl, char *txt)
{
    if (sl->lines < SUBTITLE_MAX_LINES && txt)
    {
	int n;
        char* t;
	char* b = trim_spaces(txt);
        trim_http(b);

	// skip color and font-style change - not supported right now
	// {c:$00ffff} {y:i}
	t = strchr(b, '{');
	if (t)
	{
	    char u = toupper(t[1]);
	    if ((u == 'C' || u == 'Y') && t[2] == ':') {
		t = strchr(t + 3, '}');
		if (t)
		    b = t + 1;
	    }
	}

	n = strlen(b);

	if (sl->lines > 0 || n > 0)
	{
	    size_t sz = n + 1;
            char* e;
#ifdef HAVE_ICONV
	    char local[1024]; // at most this chars per line
	    char* out_p = local;
	    iconv_t icvsts = iconv_open("UTF-8", l->encoding);
	    if (icvsts != (iconv_t)(-1)) {
		char* in_p = b;
		size_t in_size = n;
		size_t out_size = sizeof(local) - 1;
		if ((size_t)(-1) == iconv(icvsts, (ICONV_CONST_CAST char**) &in_p, &in_size, &out_p, &out_size))
		    printf("subtitles: iconv convert error %s\n", b);
		iconv_close(icvsts);
		//printf("SZ %d -> %d    (%s)\n", sz, out_p - b + 1, l->encoding);
		b = local;
		sz = out_p - b + 1;
	    }
	    else
		printf("subtitles: iconv open error\n");
#endif

	    e = (char*) malloc(sz);
	    if (e)
	    {
		memcpy(e, b, sz - 1);
		e[sz - 1] = 0;
		sl->line[sl->lines++] = e;
	    }
	}
    }
    else
	printf("Too many subtitle lines: %d ignoring: %s\n", sl->lines, txt);

    if (sl->end < sl->start)
	// some subtitles end with {XX}{0} - confusing our searcher
	sl->end = sl->start + 1;
    //printf("ADDLINE %d >%s< %d %d\n", l->lines, txt, l->start, l->end);;
}

static void add_line_columned(subtitles_t* l, subtitle_line_t* sl, char *text)
{
    for (;;)
    {
	char* s = strchr(text, '|');
	//printf("text %s  %p\n", text, s);
	if (s == NULL)
	{
	    add_line(l, sl, text);
            break;
	}
	*s++ = 0;
	add_line(l, sl, text);
	text = s;
    }
}

static void add_sub_line(subtitles_t* l, subtitle_line_t* sl)
{

    if (l->allocated <= (l->count + 1))
    {
	l->allocated += 200;
	//printf("Realloc %p   %d\n", l->subtitle, sizeof(*sl) * l->allocated);
	l->subtitle = (subtitle_line_t*) realloc(l->subtitle, sizeof(*sl) * l->allocated);
    }

    if (l->subtitle)
    {
	if (l->count > 0)
	{
	    // this trick is for weird subtitles
	    // i.e. joined two files subtitles from 2 CDs
	    sl->start += l->time_diff;
	    sl->end += l->time_diff;
	    if (l->subtitle[l->count - 1].end > sl->start)
	    {
		//printf("TIMEDIFF  %d   %d   s:%d  e:%d\n", l->time_diff, sl->start, l->subtitle[l->count - 1].start, l->subtitle[l->count - 1].end);
		if (l->subtitle[l->count - 1].start < sl->start)
                    l->subtitle[l->count - 1].end = sl->start - 1;

		l->time_diff = l->subtitle[l->count - 1].end - sl->start;
		if (l->time_diff < 1000000)
		{
		    l->time_diff = 0;
		    //return; // discard
		}
		//printf("TM %d  %s\n", l->time_diff, sl->line[0]);
	    }
	}
        memcpy(&l->subtitle[l->count], sl, sizeof(*sl));
        l->count++;
    }
}

static const int SAMI_END = 100;
static int parse_SAMI(subtitles_t* l, subtitle_line_t* sl, char* s, int state, char* temp)
{
    char *p, *q;

    p = temp + strlen(temp);

    for (;;)
    {
	//printf("SAMI %d  %s\n", state, s);
	switch (state) {

	case 0: /* find "START=" */
	    s = strstr(s, "Start=");
	    if (s)
	    {
		sl->start = strtol(s + 6, &s, 0);
		state = 1;
		continue;
	    }
	    break;

	case 1: /* find "<P" */
            s = strstr (s, "<P");
	    if (s)
	    {
		s += 2;
		state = 2;
		continue;
	    }
	    break;

	case 2: /* find ">" */
            s = strchr (s, '>');
	    if (s)
	    {
		s++;
		p = temp;
		state = 3;
		continue;
	    }
	    break;

	case 3: /* get all text until '<' appears */
	    if (*s == '<')
		state = 4;
	    else if (!strncasecmp(s, "&nbsp;", 6))
	    {
		*p++ = ' ';
		s += 6;
	    }
	    else if (!strncasecmp(s, "<br>", 4) || *s == 0)
	    {
		*p = 0;
		add_line(l, sl, temp);
		p = temp;
		if (*s == 0)
                    break;
		s += 4; // <br>
	    }
	    else
		*p++ = *s++; // copy
	    continue;

	case 4: /* get end or skip <TAG> */
	    q = strstr(s, "Start=");
	    if (q) {
		sl->end = strtol(q + 6, &q, 0) - 1;
		*p = 0;
		state = SAMI_END; // finished one subtitle
		break;
	    }
	    s = strchr(s, '>');
	    if (s)
	    {
		s++;
		state = 3;
		continue;
	    }
	    break;
	}
        // when we get here we need next line -> break main loop
	break;
    }
    *p = 0;
    return state;
}


#define IS_SUB(type) (!stype || (stype == type))

#define SHOWSUBTIME  5     /* approx. 5 seconds */

static void subtitle_reread(subtitles_t* l)
{
    int dummy, sh, sm, ss, su, eh, em, es, eu;
    int c = 0;
    int line = 0;
    subtitle_t stype = SUBTITLE_UNSELECTED;
    char bfr[1024];
    fpos_t fpos;
    int tdiff = 0;
    float mpsub_position = 0;

    FILE* file = fdopen(l->fd, "rt");
    if (!file)
    {
	perror("subtitle open:");
        return;
    }

    // ORDER OF 'if's IS IMPORTANT!!!!!
    // this piece of code seem to be looking a bit complicated
    // but it is quite efficient and allows to read even subtitles
    // with different types in the same file :) (not very useful thought :))

    while (read_line(bfr, sizeof(bfr), file))
    {
	subtitle_line_t sl;
	int n = 0;

        memset(&sl, 0, sizeof(sl));
	//printf("LINE  %d  %d  %s\n", line, stype, bfr);
	// MicroDVD (.sub)
	if (IS_SUB(SUBTITLE_MICRODVD))
	{
	    if (sscanf(bfr, "{%d}{%d}%n", &sl.start, &sl.end, &n) == 2)
	    {
		stype = SUBTITLE_MICRODVD;

		if (sl.end < sl.start)
		{
		    if (l->fps > 0. && sl.end / l->fps > SHOWSUBTIME)
			sl.end = SHOWSUBTIME;
                    sl.end = (int)(sl.start + sl.end * l->fps);
		}
		add_line_columned(l, &sl, bfr + n);
		skip_to_eol(file);
	    }
	    else if (sscanf(bfr, "{%d}{}%n", &sl.start, &n) == 1)
	    {
		stype = SUBTITLE_MICRODVD;

		sl.end = (int)(sl.start + SHOWSUBTIME * l->fps);
		add_line_columned(l, &sl, bfr + n);
		skip_to_eol(file);
	    }
	}

	// SubRipper
	if (IS_SUB(SUBTITLE_SUBRIP)
	    && sscanf(bfr, "%d:%d:%d.%d,%d:%d:%d.%d",
		      &sh, &sm, &ss, &su, &eh, &em, &es, &eu) == 8)
	{
	    stype = SUBTITLE_SUBRIP;

	    sl.start = get_subtime(sh, sm, ss, su);
	    sl.end   = get_subtime(eh, em, es, eu);
	    // now there can be 0, 1 or 2 lines of subtitles... ended by a newline
	    while (read_line(bfr, sizeof(bfr), file))
	    {
		// we skip whitespace and check whether the line's empty
		char* p = skip_spaces(bfr);
                char* out;
		if (*p == '\0')
		    break;	// this was a blank line -> end of titles here

		// replace [br] [BR] -> |
		out = p = bfr;
		while (*p != 0)
		{
		    if (!strncasecmp(p, "[br]", 4))
		    {
                        *p = 0;
			p += 4;
			add_line(l, &sl, out);
                        out = p;
		    }
                    else
			p++;
		}
                if (out < p)
		    add_line(l, &sl, out);
	    }
	}

	// vplayer format
	if (IS_SUB(SUBTITLE_VPLAYER)
            && sscanf(bfr, "%d:%d:%d:%n", &sh, &sm, &ss, &n) == 3)
	{
            stype = SUBTITLE_VPLAYER;

	    sl.start = get_subtime(sh, sm, ss, 0);
	    sl.end = sl.start + SHOWSUBTIME * 1000;

	    add_line_columned(l, &sl, &bfr[n]);
	    skip_to_eol(file);
	}

        // Aqt (not sure who this is supposed to work)
	if (IS_SUB(SUBTITLE_AQT)
	    && sscanf(bfr, "-->> %d", &sl.start) == 1)
	{
	    int lineadded = 0;
	    unsigned int chr = 0;
            stype = SUBTITLE_AQT;

	    while (read_line(bfr, sizeof(bfr), file))
	    {
		// we skip whitespace and check whether the line's empty
		if (lineadded)
		{
		    char *p = skip_spaces(bfr);
		    if (*p == '\0')
			break;	// this was a blank line -> end of titles here
		}
		// line's not empty, we add as much as we can to name
		add_line(l, &sl, bfr);
                chr += strlen(bfr);
		lineadded =  1;
	    }
	    sl.end = sl.start + chr * 5;
	}

        // SAMI (.smi)
	if (IS_SUB(SUBTITLE_SAMI)
	    && (stype || strstr(bfr, "SAMI")))
	{
            char temp[sizeof(bfr)];
	    int state = 0;
	    stype = SUBTITLE_SAMI;
	    for (;;)
	    {
		state = parse_SAMI(l, &sl, bfr, state, temp);
		if (state == SAMI_END)
		{
		    fsetpos(file, &fpos); // back to the begining of the current line
		    break;
		}
		fgetpos(file, &fpos);
		if (!read_line(bfr, sizeof(bfr), file))
                    break;
	    }
	    if (feof(file))
		break;
	    fgetpos(file, &fpos);
	}
	// OldSubViewer (.srt) {%d:%d:%d}{%d:%d:%d}
	// SubViewer (.srt)
	if (IS_SUB(SUBTITLE_SUBVIEWER)
	    && sscanf(bfr, "%d", &dummy) == 1)
	{
	    // Skip this buffer with only one number
	    // this should be tested as last case
	    // as it destructs the line in buffer !!!
	missnl:
	    if (read_line(bfr, sizeof(bfr), file)
		// sscanf(bfr, "%02d%*c%02d%*c%02d%*c%03d --> %d:%d:%d,%d",
		&& sscanf(bfr, "%d:%d:%d,%d --> %d:%d:%d,%d",
			  &sh, &sm, &ss, &su, &eh, &em, &es, &eu) == 8)
	    {
		int lineadded = 0;

                stype = SUBTITLE_SUBVIEWER;
		sl.start = get_subtime(sh, sm, ss, su);
		sl.end   = get_subtime(eh, em, es, eu);
		// now there can be 0, 1 or 2 lines of subtitles...
		// usually ended by a newline - but sometime this new line is missing
		while (read_line(bfr, sizeof(bfr), file))
		{
                    int dummy1;
		    // we skip whitespace and check whether the line's empty
		    if (lineadded)
		    {
			char *p = skip_spaces(bfr);
			if (*p == '\0')
			    break;	// this was a blank line -> end of titles here
		    }
		    // if line's not empty, we add as much as we can to name
                    // but we check if there is not begin of the new subtitle
		    if (sscanf(bfr, "%d", &dummy1) == 1 && ((dummy + 1) == dummy1))
		    {
                        int i;
			//printf("DUMM %d  %d\n", dummy, dummy1);
                        dummy = dummy1;
			add_sub_line(l, &sl);
			for (i = 0; i < sl.lines; i++)
			    sl.line[i] = NULL;
                        // no freeing lines -just reset counter
                        sl.lines = 0;
			goto missnl;
		    }
		    add_line(l, &sl, bfr);
		    lineadded = 1;
		}
	    }
	}
	if (IS_SUB(SUBTITLE_MPSUB)
	    && (stype || strstr(bfr, "FORMAT=TIME")
		//|| (stype = SUBTIsscanf(line, "FORMAT=%d", &i) == 1)
	       ))
	{
	    float a, b;

	    stype = SUBTITLE_MPSUB;

	    if (sscanf(bfr, "%f %f", &a, &b) == 2)
	    {
		int lineadded = 0;
		mpsub_position += a * 1000.0;
		sl.start = (int)mpsub_position;
		mpsub_position += b * 1000.0;
		sl.end = (int)mpsub_position;
		while (read_line(bfr, sizeof(bfr), file))
		{
		    // we skip whitespace and check whether the line's empty
		    if (lineadded)
		    {
			char *p = skip_spaces(bfr);
			if (*p == '\0')
			    break;	// this was a blank line -> end of titles here
		    }
		    add_line(l, &sl, bfr);
		    lineadded = 1;
		}
	    }
	}
        if (!stype)
	{
	    unsigned errch = 0;
	    unsigned i;
	    for (i = 0; bfr[i]; i++)
	    {
		if (bfr[i] < ' ' && bfr[i] != '\t')
		    errch++;
	    }
	    if (errch > 10)
                break; // these are not ASCII subtitles
	    //printf("ERR  %d  %d\n", errch, i);

	    if (line++ > 100)
                break; // at most 100 unidentified lines
	}
        if (sl.lines > 0)
	    add_sub_line(l, &sl);
    }

    fclose(file);

    l->type = stype;
    switch (stype)
    {
    case SUBTITLE_MICRODVD:
	/* formats which are using frames */
	l->frame_based = 1;
	break;
    default:
        l->frame_based = 0;
    }
}

// SubRip example
//
// [INFORMATION]
// [TITLE]Me, myself and Irene
// [AUTHOR]Mysak
// [SOURCE]Subtitles captured by SubRip 0.93b
// [PRG]
// [FILEPATH]
// [DELAY]0
// [CD TRACK]0
// [COMMENT]
// [END INFORMATION]
// [SUBTITLE]
// [COLF]&HFFFFFF,[STYLE]bd,[SIZE]18,[FONT]Arial
// 00:00:01.02,00:00:07.00
// Titulky beta verze by My..
//
// 00:00:07.03,00:00:10.59
// text[br]text

static int test_filename_suffix(const char* filename, char** f)
{
    static const char* exts[] =
    {
        "",
	".sub", ".SUB",
	".srt", ".SRT",
	".txt", ".TXT",
	".aqt", ".AQT",
	".smi", ".SMI",
	".utf", ".UTF",
	/*".rt", ".RT",
	".ssa", ".SSA",*/
	NULL
    };
    const char** s;
    size_t n = strlen(filename);
    char* fn = (char*) malloc(n + 8);
    int fd = -1;

    for (s = exts; fn && *s; s++)
    {
	strcpy(fn, filename);
	strcpy(fn + n, *s);

	//printf("test %s  %s %s\n", fn, filename, *s);
        fd = open(fn, O_RDONLY);
	if (fd >= 0)
            break;
    }

    if (f && fd >= 0)
	*f = fn;
    else
	free(fn);
    return fd;
}


/*
 * create name for subtitle file
 */
int subtitle_filename(const char* filename, char** opened_filename)
{
    int fd;
    char* fn, *fdup;
    size_t n;

    if (!filename)
	return -1;

    fd = test_filename_suffix(filename, opened_filename);
    fdup = strrchr(filename, '.');
    if (fdup == NULL || strchr(fdup, '/') || fd >= 0)
	return fd;

    n = fdup - filename;
    fdup = (char*) malloc(n + 1);
    if (!fdup)
	return -1;

    memcpy(fdup, filename, n);
    fdup[n] = 0;

    fd = test_filename_suffix(fdup, opened_filename);
    free(fdup);
    if (fd < 0)
	fd = test_filename_suffix(filename, opened_filename);

    return fd;
}

subtitles_t* subtitle_open(int fd, double fps, const char* enc)
{
    char* subname;
    subtitles_t* l = (subtitles_t*) malloc(sizeof(*l));
    if (!l)
	return NULL;
    memset(l, 0, sizeof(*l));

    l->fd = fd;
    l->fps = fps;
    if (!enc || strstr(enc, "default"))
	enc = nl_langinfo(CODESET);
    l->encoding = strdup(enc);
    subtitle_reread(l);

    return l;
}
