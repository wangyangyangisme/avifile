# This file you have to edit. Change the name here

#this is probably wrong.
#mcopclassdir variable should point somewhere inside KDE installation,
#I am not sure that .mcopclass will be found by aRts here.
mcopclassdir			=	$(libdir)/mcop

#mcopclass_DATA			=	AVIPlayObject.mcopclass
mcopclassDATA			=	AVIPlayObject.mcopclass
EXTRA_DIST			=	playobjects.idl $(mcopclassDATA)
					$(libarts_aviplay_plugin_la_SOURCESx)
noinst_HEADERS			=	aviPlayObject_impl.h
AM_CPPFLAGS = $(ARTSC_INCLUDE) $(ARTSC_INCLUDE)/../arts
BUILT_SOURCES			=	
#BUILT_SOURCES			=	playobjects.h playobjects.cc

# do not build
#lib_LTLIBRARIES 		=	libarts_aviplay_plugin.la

#playobjects.cc			:	playobjects.h
#playobjects.h			:	playobjects.idl
#		echo "broken plugin"
#		mcopidl $(ARTS_ALL_INCLUDES) playobjects.idl

libarts_aviplay_plugin_la_SOURCESx	=  	$(BUILT_SOURCES) \
						aviPlayObject_impl.cpp
libarts_aviplay_plugin_la_LDFLAGSx	=   \
						-module \
						-L../../lib
libarts_aviplay_plugin_la_LIBADDx	=	 -laviplay -L../../lib/.libs

MAINTAINERCLEANFILES = Makefile.in
