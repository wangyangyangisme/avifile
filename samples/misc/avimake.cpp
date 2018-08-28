//                              -*- Mode: C++ -*- 
// avimake.cc -- create a DIVX movie out of a slew of JPEGs
// 
// $Id: avimake.cpp,v 1.11 2004/03/10 09:12:29 kabi Exp $
// 
// Copyright (C) 2001  Tom Pavel <pavel@alum.mit.edu>
// 
// Creator         : Tom Pavel  <tom>     Sun Feb  4 01:23:17 2001
// Modifier        : Tom Pavel  <tom>     Sun May 13 00:35:21 2001
// Update Count    : 45
// Status          : Unknown, Use with caution!
// 


#include <avifile.h>
#include <aviplay.h>
#include <avm_fourcc.h>
#include <avm_except.h>
#include <version.h>

#include <unistd.h>		// for getopt()
#include <string.h>		// for memset()
#include <stdlib.h>		// atoi()
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>

extern "C" {
#undef HAVE_STDLIB_H    // prevents warning...
#include <jpeglib.h>
}

#include <iostream>

using namespace std;

void fillInfo(const char* fname, BITMAPINFOHEADER* bi)
{
    // open and read fname as JPG file
    // use jpg lib...
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    FILE* infile = fopen(fname, "rb");
    if (infile  == NULL) {
	cerr << "can't open " <<  fname
	     << ": " << strerror(errno) << endl;
	exit(1);
    }
    jpeg_stdio_src(&cinfo, infile);

    jpeg_read_header(&cinfo, TRUE);

    memset(bi, 0, sizeof(*bi));
    bi->biSize = sizeof(*bi);
    bi->biWidth = cinfo.image_width;
    bi->biHeight = cinfo.image_height;
    bi->biSizeImage = bi->biWidth * bi->biHeight * 3;
    bi->biPlanes = 1;
    bi->biBitCount = 24;

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
}


void addFrame(const char* fname, avm::IVideoWriteStream* outVidStr)
{
    // open and read fname as JPG file
    // use jpg lib...
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    FILE * infile = fopen(fname, "rb");
    if (infile  == NULL) {
	cerr << "can't open " <<  fname 
	     << ": " << strerror(errno) << endl;
	exit(1);
    }
    jpeg_stdio_src(&cinfo, infile);

    jpeg_read_header(&cinfo, TRUE);
    cinfo.out_color_space = JCS_RGB;
    cinfo.quantize_colors = FALSE; 

    unsigned int line_size = 3 * cinfo.image_width;
    JSAMPLE* line = new JSAMPLE[line_size];
    JSAMPLE* data = new JSAMPLE[line_size*cinfo.image_height];

    jpeg_start_decompress(&cinfo);

    while (cinfo.output_scanline < cinfo.output_height) {
	// The damn BMP files are backwards! 
	// (first row at bottom of image, and BGR instead of RGB):
        int lines = jpeg_read_scanlines(&cinfo, &line, 1);
	assert (lines == 1);
	
	JSAMPLE* ptr = & data[line_size *
			      (cinfo.image_height-cinfo.output_scanline - 1)];
	for (JSAMPLE* src = line; src < &line[line_size]; src += 3, ptr += 3) {
	    ptr[0] = src[2];
	    ptr[1] = src[1];
	    ptr[2] = src[0];
	}
    }

    delete [] line;
    jpeg_finish_decompress(&cinfo);


    // create frame from 24-bit image array.
    avm::CImage frame(data, cinfo.output_width, cinfo.output_height);

    outVidStr->AddFrame(&frame);

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    delete [] data;
}



void
Usage (const char* progname)
{
    cerr << "usage: " << progname
	 << " [-h] [-o outfile] [-r fr] [-q quality]"
	 << endl;
    exit(1);
}


int
main (int argc, char* argv[])
try
{
    int debug = 0;
    const char* outFn = "out.avi";
    int fr = 5;			// default to 5 fps
    int quality = 10000; 	// units = .01 percent

    int ch;
    while ((ch = getopt(argc, argv, "dho:r:q:")) != EOF) {
        switch ((char)ch) {
        case 'd':
            ++debug;
	    break;

	case 'o':
	    outFn = optarg;
	    break;

	case 'r':
	    fr = atoi (optarg);
	    if (fr == 0)
		Usage (argv[0]);
	    break;

	case 'q':
	    quality = atoi (optarg);
	    if (quality == 0)
		Usage (argv[0]);
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
        cout << "This binary was compiled for Avifile ver. "
	     << AVIFILE_VERSION
	     << ", but the library is ver. "
	     << GetAvifileVersion()
	     << ". Aborting."
	     << endl;
        return 0;
    }   

    // Do the real work
    avm::IWriteFile* outFile = avm::CreateWriteFile(outFn);
    BITMAPINFOHEADER bi;
    //FOURCC codec = fccDIV3;
    fourcc_t codec = fccMP42;
    //FOURCC codec = fccIV32;
    //FOURCC codec = fccCVID;
    

    // Use the first file to pick the image sizes:
    fillInfo (*argv, &bi);

    avm::IVideoWriteStream* vidStr =
	outFile->AddVideoStream(codec, &bi, 1000000/fr);

    vidStr->Start();
    while (argc > 0) {
	addFrame (*argv, vidStr);
        --argc;
        ++argv;	
    }
    vidStr->Stop();

    // Close the outFile and write out the header, etc.
    delete outFile;
}

catch(FatalError& error) {
    error.Print();
}   
