SUBDIRS = . admin m4 ffmpeg drivers lib plugins bin \
 doc include debian libavqt player samples

EXTRA_DIST = INSTALL autogen.sh avifile.spec.in avifile.pc.in \
 $(pkgdoc_DATA) $(pkgconfig_DATA)

bin_SCRIPTS = avifile-config

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = avifile.pc

pkgdocdir = $(datadir)/$(PACKAGE)/doc
pkgdoc_DATA = README COPYING ChangeLog

mostlyclean-generic:
	-rm -f core build-stamp configure-stamp `find . -name '#*' -o \
		-name '.\#*' -o -name '.*~' -o -name '*~' -o -name 'core*'`
	-rm -rf core `find . -o -name autom4te.cache`
	-rm -f $(PACKAGE)_$(VERSION).tar.gz
	-rm -f $(distdir).tar.gz $(PACKAGE).tgz package_descriptions
	-rm -rf $(distdir) debian/libavifile*[0-9]

## not really usable now -name '.deps'

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure config.cache \
 config.log config.status configure.ac config.h.in confdefs.h libtool \
 avifile-config avifile.spec avifile.pc stamp-h2.in

# Run ldconfig after installing the library:
install-hook:
	-ldconfig

#install-exec-local:
#	$(INSTALL_SCRIPT) -m 0755 $(srcdir)/avifile-config $(DESTDIR)$(bindir)
.SILENT: help

help:
	echo "Useful targets:"
	echo "  make [all]      : compile all the avifile parts"
	echo "  make install    : install 'avifile' stuff on your system"
	echo "  make clean      : cleanup compilation stuff"
	echo "  make distclean  : remove data relative to your system"
	echo "Note: you may try to run 'autogen.sh',"
	echo "      and then run './configure' before trying to compile"
	echo "      if you have some compilation troubles"
