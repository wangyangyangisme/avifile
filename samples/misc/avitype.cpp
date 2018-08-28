//                              -*- Mode: C++ -*-
// avitype.cpp -- Print info about a .AVI file
//
// $Id: avitype.cpp,v 1.12 2003/05/26 08:57:33 kabi Exp $
//
// Copyright (C) 2001  Tom Pavel <pavel@alum.mit.edu>
//
// Creator         : Tom Pavel  <tom>     Sat Feb  3 01:27:20 2001
// Modifier        : Tom Pavel  <tom>     Sun May 13 00:56:10 2001
// Update Count    : 34
// Status          : Unknown, Use with caution!
//



#include <avifile.h>
#include <aviplay.h>
#include <avm_fourcc.h>
#include <avm_except.h>
#include <utils.h>
#include <version.h>

#include <unistd.h>		// for getopt()
#include <stdio.h>
#include <stdlib.h>

static void Usage(const char* progname)
{
    fprintf(stderr, "Usage: %s  [-h] file1 file2 ...\n",  progname);
    exit(1);
}

static void printFile(const char* fname)
{
    avm::IReadFile* aviFile = avm::CreateReadFile(fname);
    if (aviFile == 0)
	return;
#if 0
    MainAVIHeader hdr;
    if (aviFile->GetHeader(&hdr, sizeof(hdr)) != 0)
	return;

    cout << endl;
    cout << "AVI file:    " << fname << endl;
    cout << "    Frames:  " << hdr.dwTotalFrames
	 << " of " << hdr.dwWidth << "x" << hdr.dwHeight << endl;
    cout << "    Rate:    " << setprecision(4)
	 << 1e6/(double)hdr.dwMicroSecPerFrame << " frame/sec"
	 << " or " << hdr.dwMaxBytesPerSec/1000.0 << " kB/sec" << endl;
    cout << "    Streams: " << hdr.dwStreams << " ("
	 << aviFile->VideoStreamCount() << "V + "
	 << aviFile->AudioStreamCount() << "A)"
	 << endl;

    IAviReadStream* vidStream = aviFile->GetStream (0, AviStream::Video);
    if (vidStream == 0) {
	cout << "Bad Video Stream" << endl;
	return;
    }
    AVIStreamHeader header;
    vidStream->GetHeader(&header, sizeof(header));
    cout << "    Video:   " << avm_fccToString(header.fccType)
	<< " " << avm_fcc_name(header.fccHandler)
	<< endl;

    IAviReadStream* audStream = aviFile->GetStream (0, AviStream::Audio);
    if (audStream == 0) {
	cout << "Bad Audio Stream" << endl;
	return;
    }
    WAVEFORMATEX wvFmt;
    if (audStream->GetAudioFormatInfo(&wvFmt, 0) != 0) {
	cout << "Failed to decode Audio Format" << endl;
	return;
    }
    cout << "    Audio:   " << avm_wave_format_name(wvFmt.wFormatTag)
	 << " " << wvFmt.wBitsPerSample << "-bit "
	 << wvFmt.nChannels << "-chan "
	 << wvFmt.nSamplesPerSec << " Hz" << endl;
#endif

}

int main(int argc, char* argv[])
try
{
    int debug = 0;

    int ch;
    while ((ch = getopt(argc, argv, "dhl:")) != EOF) {
	switch ((char)ch) {
	case 'd':
	    ++debug;
	    break;

	case 'h':
	case '?':
	default:
	    Usage (argv[0]);
	}
    }

    argc -= optind;
    argv += optind;
    if (argc < 1) Usage (argv[0]);


    // Standard AVIlib sanity check:
    if ( GetAvifileVersion() != AVIFILE_VERSION) {
	printf("This binary was compiled for Avifile ver. %x"
	       ", but the library is ver. %x. Aborting.\n",
	       AVIFILE_VERSION, GetAvifileVersion());
	return 0;
    }

    while (argc > 0) {
	printFile (*argv);
	--argc;
	++argv;
    }

}
catch(FatalError& error) {
    error.Print();
}

