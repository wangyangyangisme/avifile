SUBDIRS = dmo dshow wine

if AMM_USE_WIN32
noinst_LTLIBRARIES = libloader.la
endif

noinst_HEADERS = \
 com.h \
 driver.h \
 ext.h \
 ldt_keeper.h \
 loader.h \
 registry.h \
 win32.h \
 wineacm.h

libloader_la_SOURCES = \
 afl.c \
 driver.c \
 elfdll.c \
 ext.c \
 ldt_keeper.c \
 module.c \
 pe_image.c \
 pe_resource.c \
 registry.c \
 resource.c \
 vfl.c \
 win32.c

libloader_la_LIBADD = \
 dmo/libdmo.la \
 dshow/libdshow.la

# gcc-3.0 produces buggy code for acmStreamOpen() with
# "-O3 -fomit-frame-pointer" or "-O2 -fomit-frame-pointer
# -finline-functions -frename-registers" (code is OK with sole -O2),
# the bad code accesses parameters via %ebp without setting up a
# propper %ebp first!
# -fno-omit-frame-pointer works around this gcc-3.0 bug.  gcc-2.95.2 is OK.
#
# See "view problem report" #3480 here: http://gcc.gnu.org/cgi-bin/gnatsweb.pl

WINFLAGS = -DWIN32_PATH=\"$(WIN32_PATH)\" -D__WINE__ -DUNICODE \
 -Ddbg_printf=__vprintf -DTRACE=__vprintf
# -D__NO_STRING_INLINES

# to help detect missing/inconsistent __stdcall function attributes in
# avifile's wine code:
AM_CPPFLAGS = $(LTNOPIC) -DAVIFILE $(WINFLAGS) -Wmissing-prototypes -Wimplicit-function-declaration

# -mno-omit-leaf-frame-pointer added to workaround bug with WMV8 and gcc-3.2
AM_CFLAGS = -fno-omit-frame-pointer 
#-mno-omit-leaf-frame-pointer
CFLAGS = $(WIN32_CFLAGS)
#-O2 -ffast-math
#-fno-omit-frame-pointer
#-O3 -fomit-frame-pointer
#

#AS = gcc
#ASFLAGS = -g -O2

LIBS =

# not needed anymore
EXTRA_DIST = stubs.s

# .text section has been changed to .data section
# so we eliminate .rel.text section from library
# it works this way too
# we actually don't need this line
#stubs.lo: stubs.s
#	$(CC) -c $(srcdir)/stubs.s -o stubs.lo

#win32.lo: win32.c
#	$(CC) -O2 -fno-inline -fno-strict-aliasing $(DEFS) $(WINFLAGS) -c $< -o $@

MAINTAINERCLEANFILES = Makefile.in
