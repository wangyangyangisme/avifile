
  This archive contains version 0.6 of the Avifile library, 
  an AVI player and a couple of sample apps.

=== Requirements ===
  Linux ( should work on all modern distributions, kernel 2.2.0 or newer )
  or FreeBSD >=4.1 ( thanks to Hiroshi Yamashita ).
  Win32 binaries for compression/decompression of video ( available from site ).
  Some test programs expect to find 384*288*24 BMP file './uncompr.bmp'.
  Processor: Intel Pentium or compatible. MMX is strongly recommended.
  C++ compiler, compatible with latest C++ standard ( such as gcc 2.95.2 ).

=== Requirements for 'aviplay' sample ===
  XFree86 3.x or 4.x.
  Qt library ver. 2.0 or newer ( NOT 1.44! ). If you've built Qt from sources,
  environment variable QTDIR should point at root of installation or you should
  specify path to Qt in ./configure options.
  SDL library ver. 1.0 or newer. Version 1.1.7 may cause problems with
  compilation and thus isn't recommended to use 
  - use 1.2 or better if you can.

=== Requirements for 'qtvidcap' sample ===
  Video capture card with V4L1 interface driver. Tested on bttv:
  http://www.in-berlin.de/User/kraxel/bttv.html. For Miro DC10+ cards
  you should read carefully README from driver package ( section related
  to uncompressed video capture ).

  I suppose you know what you are doing if you are trying to use it, so no
  documentation about capturing process is available yet.

 Installation ( ideal variant ):
 (See INSTALL for installation of CVS version)

 mkdir /usr/lib/win32
 unzip ./binaries.zip -d /usr/lib/win32
 tar zxf ./avifile-xxx.tar.gz
 cd avifile

 #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 # for the latest CVS release - running 'autogen.sh' is strongly recommended
 # it will create correct configure script for your system
 #
 # you may use CVS snapshot which contains all necessary scripts together 
 # with configure script

 ./configure
 make
 make install
 aviplay <your-file>

  You can use 'make install-strip' instead of 'make install' if you are worried
about disk usage. It'll give you additional 3-4 MB of disk space, but you won't
be able to produce gdb backtraces ( see below, section Reporting Bugs ).

  By default it installs main shared library into directory /usr/local/lib,
you should make sure that /usr/local/lib exists in search part of dynamic
linker ( file /etc/ld.so.conf and environment variable LD_LIBRARY_PATH ).


  ./configure options:

 --enable-release	Turn on optimisations. Does not affect
			player performance. (also disables compilation of
                        debug info so its much harder to detect reason
                        for crash)
 --disable-sdl		Disables SDL code. Not recommended.
 --disable-qt		Disables Qt utilities. Not recommended
 --prefix=<your-directory>
     			Where to install library. Default is /usr/local.
 --with-qt-dir=<your-directory>
 --with-qt-includes=<your-directory>
 --with-qt-libraries=<your-directory>
                        Specify these options if ./configure fails
			to find your Qt installation, if you receive
			strange Qt-related errors or if you want to
			build 'aviplay' for an alternative version of Qt.

  PERFORMANCE:

   Aviplay tries to be relatively efficient, but it's still very CPU-intensive
product. It's hard to tell what are its minimum requirements. For playing of
DivX ;-) AVI files in 720x480 you'll need P2-300 ( with good video card ) or
P3-600 ( with bad one ). For 384x288 DivX ;-) lower limit is about P200MMX to
P2-400. Check doc/VIDEO_PERFORMANCE if you are having problems with performance.

   Player will use 2D hardware acceleration if it's available in your system.
As of 11/25/2000, you'll need the following:

    * XFree86 4.0 or newer  (Xvideo hardware accelerated extension)
    * One of the following video cards:
	- Matrox G400.
	- Any NVIDIA card with installed binary driver,
	that is available from http://www.nvidia.com.
        - ATI newer cards (e.g. Radeon should work)

   It may also work with the other video cards, but you will need
latest driver. Refer to XFree86 4.0.2 documentation for details
about XVideo extension support in various card drivers.

   Hardware acceleration reduces ( by 20-30% in 16-bit color )
CPU load and allows to perform hardware scaling of the picture. Here are
CPU load numbers, measured on Pentium II-400(480)/Riva TNT, 16 bpp,
384x288 movie at 25 fps ( by Sergey Zhitomirsky, szh (at) 7ka,mipt,ru ):

ZOOM       HARDWARE          SOFTWARE
1x         20-25%	     25-30%
2x         20-25% 	     40-45%
1024x768   20-25%	     45-50%

    Acceleration can be disabled from 'config' player menu.

  REPORTING BUGS:

   Please include output log and gdb backtrace in case of segfault or lock-up.
Backtrace is desired for all running threads:
  $ gdb aviplay
  (gdb) run <your-file>
	    ( type 'cont' if gdb stops with non-fatal messages )
  (gdb) info threads
  (gdb) thread 1
  (gdb) bt
  (gdb) thread 2
  ...

   The amount of these data is the only significant factor in catching your bug.
I can't understand what's going wrong for you if you only tell me that 'aviplayer
has crashed when closing'.

 Your comments are welcome, but please look through the FAQ before
posting them. It's up-to-date version is available at http://divx.euro.ru/faq.htm.
 Mail your questions and at divx@euro.ru.

   SCHEDULING PRIORITIES

   For the best video experience it's useful to understand Linux scheduler
and also it's good to know how aviplay internaly works. So here is some short
overview:

 Player is divided into several threads - for prefetching data, decoding
audio, playing audio, decoding video, playing video and GUI has also its
own thread. Also X11 events have its own thread. So you see there is
quite a lot of them - most of them are idle most of the time. But some
of them have almost real-time needs. The most important one is
video thread which main purpose is it place the image from buffered queue 
to the screen at very precise time.

You would not notice 10ms delays, but anything above this create noticable
jerk in the video. So now few words about priorities - if all of these
threads have same priority then video decoder is fighting for the CPU with
video drawing thread - however if we increase priority (thus make task
less preffered for the CPU) that we increase the chance that image will
be placed on the screen at the right time. Ok now I hope you understand
the reason for lowering priority of video & audio decoding threads:
1. Video decoder is highly CPU intensive task - actually it's the only one
   which really keeps your CPU busy when you use hw accelerated rendering)
   - and all decoded images are stored in buffers (when enabled)
   thus this thread doesn't need high priorities.
2. Audio decoder takes far less CPU and as we are caching more than 1sec
   of audio we also do not need very precise timing in this process.
If these threads have lower priority that it looks like video playing thread
gets all the needed time.
   There is also another way - using FIFO scheduler - however for this reason
the player's binary has to made root.root suid (or you could even run it as
root which is even more dangerous) - this is serious problem for the
security of your system - thought the player tries to use this priority only
when changing scheduler and then drop this root privileges for the decoding
you never know what you could expect from binary-only Windows code
(in case you run this as root - then windows codec have full control over
your system!)

So before you start to ask developers why you see highly CPU intensive
task with such low priorities - read the text above first and try
to find some other places where linux scheduling is described in details.

   VIDEO OPTIONS

 * Use 2D hw acceleration
   When you have some modern graphics cards and XFree4.0 you could use
   neat Xvideo extension which provides high quality graphics output
   which could be rescaled by hardware and its also very fast as it is
   using just 16 bits for superior image quality - also decoding to
   YUV color space is usually faster

 * Set CPU quality automagicaly
   works only when buffering is activated and does the following thing:
   Aviplayer allocates several buffers for image precaching
   (currently 13 images are used). Decoding thread continually fills this
   buffer while video playing thread consumes them. The more the buffer
   is full the higher CPU quality is being set by player - of course
   when buffer is becoming empty the CPU quality is lowered.

 * Direct rendering
   In this case image is being decompressed directly into graphical memory
   if possible. This way we save one memory copy of the whole image.

 * Buffer frames
   Enables precaching of images - this is very important if you want to
   have smooth video - and is also necessary for autoquality.
   If you do not use buffering decoding is slightly faster (e.g. you should
   use it if you have really slow machine and smooth video is the last thing
   you would care about) as this way decoding is being made in the
   one memory place and the previous image doesn't have to be transfered
   in this place. But as decompression times between frames are really
   very different between each frame and sometimes they takes time for two
   video frames you can't expect any miracles.

 * Dropping frames
   Generally you should leave this option turned on all the time - only you
   want to see all the images from the movie and you don't care about
   time synchronisation.

 * Preserve video aspect ratio
   Keeps ratio for video width and height when the window is being resized.

  Using mga_vid driver (only with MGA card for now)
  Read the enclosed README in drivers directory

#internal  getrusage(2), times(2)
