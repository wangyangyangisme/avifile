SUBDIRS = avicap

man_MANS = \
 avibench.1 \
 avicap.1 \
 avicat.1 \
 avifile-config.1 \
 avimake.1 \
 aviplay.1 \
 avirec.1 \
 avirecompress.1 \
 avitype.1 \
 kv4lsetup.1

pkgdocdir = $(datadir)/$(PACKAGE)/doc
pkgdoc_DATA = FreeBSD CREDITS EXCEPTIONS KNOWN_BUGS TODO \
 VIDEO-PERFORMANCE LICENSING WARNINGS README-DEVEL

EXTRA_DIST = $(pkgdoc_DATA) $(man_MANS)

MAINTAINERCLEANFILES = Makefile.in
