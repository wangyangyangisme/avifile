#plustest qualtest
# ae test plustest imtest qualtest
#asfdump_SOURCES=asfdump.cpp
#asftest_SOURCES=asftest.cpp
avibench_SOURCES=benchmark.cpp
avitest_SOURCES=avitest.cpp
avicat_SOURCES=avicat.cpp
avitype_SOURCES=avitype.cpp

extractor_SOURCES=extractor.cpp
test_SOURCES=test.cpp
#plustest_SOURCES=plustest.cpp
#imtest_SOURCES=imtest.cpp
#qualtest_SOURCES=qualtest.cpp
#ae_SOURCES=ae.cpp
LIBRARY = ../../lib/libaviplay.la $(SDL_LIBS) $(XFT_LIBS) $(X_LIBS)
#asftest_LDADD = $(LIBRARY) AsfReadHandler.o AsfStreamSeekInfo.o
#$(LIBSOCKET) $(LIBNSL)
avitest_LDADD = $(LIBRARY)
avibench_LDADD = $(LIBRARY)
avicat_LDADD = $(LIBRARY)
avitype_LDADD = $(LIBRARY)
extractor_LDADD = $(LIBRARY)

if AMM_USE_JPEGLIB
avimake_SOURCES = avimake.cpp
avimake_LDADD = $(LIBRARY) -ljpeg
PROG_AVIMAKE = avimake
else
PROG_AVIMAKE =
endif

bin_PROGRAMS = avibench avicat avitype $(PROG_AVIMAKE)
check_PROGRAMS = avitest extractor test
#asfdump asftest 
EXTRA_DIST =  imtest.cpp plustest.cpp qualtest.cpp test.cpp

test_LDADD = $(LIBRARY)
#plustest_LDADD=$(LIBRARY)
#imtest_LDADD=$(LIBRARY)
#qualtest_LDADD=$(LIBRARY)
#ae_LDADD=$(LIBRARY)
AM_CPPFLAGS = $(X_CFLAGS) -I$(srcdir)/../../lib/aviread

AsfReadHandler.o: $(top_srcdir)/lib/aviread/AsfReadHandler.cpp
	$(CXXCOMPILE) -c $< -o $@
AsfStreamSeekInfo.o: $(top_srcdir)/lib/aviread/AsfStreamSeekInfo.cpp
	$(CXXCOMPILE) -c $< -o $@

MAINTAINERCLEANFILES = Makefile.in
