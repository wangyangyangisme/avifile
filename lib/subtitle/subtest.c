#include "subtitle.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static void subtitle_print(const subtitles_t* l)
{
    if (l != NULL)
    {
	int i;

	printf("Subtitles: %d line(s)\n", l->count);
	for (i = 0; i < l->count; i++)
	{
	    subtitle_line_t* sl = &l->subtitle[i];
	    int j;
	    //break;
	    printf("Start: %d     End: %d   (%d)\n", sl->start, sl->end, sl->lines);
	    for (j = 0; j < sl->lines; j++)
		printf("  %d: >%s<\n", j, sl->line[j]);
	}
    }
}


int main(int argc, char **argv)
{  // for testing
    int i;
    subtitles_t *subs;
    subtitle_line_t sl;
    //unsigned int ar[] = { 1165, 1493, 0 };
    //double ar[] = { 42.874, 47.1, 0 };
    double ar[] = { 22.14, 53.1, 0 };
    int fd;

    if (argc<2)
    {
	printf("\nUsage: subtitle filename.sub write.{sub|srt}\n\n");
        exit(1);
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
	fprintf(stderr, "can't open: %s\n", argv[1]);
	exit(1);
    }

    subs = subtitle_open(fd, 23.976, NULL);
    if (!subs)
    {
	printf("Couldn't load file: %s\n", argv[1]);
        exit(1);
    }

    subtitle_print(subs);
#if 0
    for (i = 0; ar[i]; i++)
    {
	if (subtitle_get(&sl, subs, ar[i]))
	{
	    int j;
	    printf("%f:  Start: %d     End: %d\n", ar[i], sl.start, sl.end);
	    for (j = 0; j < sl.lines; j++)
		printf("  %d: %s\n", j, sl.line[j]);
	}
	else
	    printf("Not found: %f\n", ar[i]);
    }
#endif

    if (argc >= 2)
    {
	subs->fps = 23.976;
	subtitle_write(subs, argv[2], SUBTITLE_MICRODVD);
    }


    subtitle_close(subs);

    //printf ("Subtitle format %s time.\n", sub_uses_time?"uses":"doesn't use");
    //printf ("Read %i subtitles, %i errors.\n", sub_num, sub_errs);
    return 0;
}
