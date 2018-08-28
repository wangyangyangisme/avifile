SUBDIRS = libdha vidix

EXTRA_DIST = mga_vid.c mga_vid.h mga_vid_test.c bswap.h config.h

# complex autodetection for presence of Modeversion and various other things

CFLAGS = -O2 -Wall $(LINUX_CFLAGS)

if AMM_USE_MGA_VID
TARGETS = mga_vid.o mga_vid_test
else
TARGETS =
endif

all: $(TARGETS)

mga_vid.o: $(srcdir)/mga_vid.c $(srcdir)/mga_vid.h
	$(CC) $(CFLAGS) -c $(basename $@).c

mga_vid_test: $(srcdir)/mga_vid_test.c $(srcdir)/mga_vid.h
	$(CC) -g -O -Wall $(INCLUDES) -o $@ $@.c

MAINTAINERCLEANFILES = Makefile.in mga_vid.o mga_vid_test
