pkglibdir = $(libdir)/$(PACKAGE)/vidix

if AMM_USE_VIDIX
VIDIX_DRIVERS = \
 libgenfb.la \
 libmach64.la \
 libmga_crtc2.la \
 libmga.la \
 libpm3.la \
 libradeon.la \
 librage128.la
endif

# just debug driver
# libnvidia.la

CLEANFILES = rage128_vid.c mga_crtc2_vid.c

all-recursive: $(CLEANFILES)

mga_crtc2_vid.c: mga_vid.c
	-echo "#define CRTC2 1" >mga_crtc2_vid.c
	-echo "#include \"mga_vid.c\"" >>mga_crtc2_vid.c

rage128_vid.c: radeon_vid.c
	-echo "#define RAGE128 1" >rage128_vid.c
	-echo "#include \"radeon_vid.c\"" >>rage128_vid.c

pkglib_LTLIBRARIES = $(VIDIX_DRIVERS)

if AMM_LINK_SHARED
LINK_LIBDHA = ../../libdha/libaviplaydha.la
endif

VIDLIBS = $(LINK_LIBDHA) -lm
VIDLDFLAGS = -module -avoid-version
libgenfb_la_SOURCES = genfb_vid.c
libgenfb_la_LIBADD = $(VIDLIBS)
libgenfb_la_LDFLAGS = $(VIDLDFLAGS)

libmach64_la_SOURCES = mach64_vid.c
libmach64_la_LIBADD = $(VIDLIBS)
libmach64_la_LDFLAGS = $(VIDLDFLAGS)

libmga_crtc2_la_SOURCES = mga_crtc2_vid.c
#libmga_crtc2_la_CFLAGS = -DCRTC2
libmga_crtc2_la_LIBADD = $(VIDLIBS)
libmga_crtc2_la_LDFLAGS = $(VIDLDFLAGS)

libmga_la_SOURCES = mga_vid.c
libmga_la_LIBADD = $(VIDLIBS)
libmga_la_LDFLAGS = $(VIDLDFLAGS)

#libnvidia_la_SOURCES = nvidia_vid.c
#libnvidia_la_LIBADD = $(VIDLIBS)
#libnvidia_la_LDFLAGS = $(VIDLDFLAGS)

libpm3_la_SOURCES = pm3_vid.c
libpm3_la_LIBADD = $(VIDLIBS)
libpm3_la_LDFLAGS = $(VIDLDFLAGS)

libradeon_la_SOURCES = radeon_vid.c
libradeon_la_LIBADD = $(VIDLIBS)
libradeon_la_LDFLAGS = $(VIDLDFLAGS)

librage128_la_SOURCES = rage128_vid.c
#librage128_la_CPPFLAGS = -DRAGE128
librage128_la_LIBADD = $(VIDLIBS)
librage128_la_LDFLAGS = $(VIDLDFLAGS)

noinst_HEADERS = \
 glint_regs.h \
 mach64.h \
 nvidia.h \
 pm3_regs.h \
 radeon.h

MAINTAINERCLEANFILES = Makefile.in
