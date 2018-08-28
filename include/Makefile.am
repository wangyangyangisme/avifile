mainincludedir = $(pkgincludedir)
maininclude_HEADERS = \
    audiodecoder.h \
    audioencoder.h \
    avifile.h \
    aviplay.h \
    avm_args.h \
    avm_cpuinfo.h \
    avm_except.h \
    avm_creators.h \
    avm_default.h \
    avm_fourcc.h \
    avm_locker.h \
    avm_output.h \
    avm_stl.h \
    configfile.h \
    cpuinfo.h \
    creators.h \
    default.h \
    except.h \
    formats.h \
    fourcc.h \
    image.h \
    infotypes.h \
    Locker.h \
    playerwidget.h \
    renderer.h \
    StreamInfo.h \
    subtitle.h \
    utils.h \
    version.h \
    videodecoder.h \
    VideoDPMS.h \
    videoencoder.h

noinst_HEADERS = \
    avm_map.h \
    plugin.h \
    mmx.h \
    vbrctrl.h

EXTRA_DIST = version.h.in
DISTCLEANFILES = version.h
MAINTAINERCLEANFILES = Makefile.in config.h.in stamp-h.in stamp-h1.in

install-hook:
	ln -s $(pkgincludedir) $(pkgincludedir)/avifile

#check_PROGRAMS = t
#t_SOURCES = t.cpp
