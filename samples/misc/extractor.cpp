/**************************************************************************

Example of IAviReadFile & IAviReadStream interface usage.
Copyright 2000 Eugene Kuznetsov (divx@euro.ru)

**************************************************************************/
#include <avm_default.h>
#include <avifile.h>
#include <aviplay.h>
#include <avm_except.h>
#include <version.h>

#include <stdio.h>
#include <stdlib.h> // exit

#define __MODULE__ "extractor"

int main(int argc, char** argv)
{
    static const int buffer_size = 20000;
    static const char* outname = "./extractor.mp3";
    FILE* f = 0;
    avm::IReadFile* ac = 0;
    avm::IReadStream* as = 0;
    uint8_t* zz = 0;
    if (GetAvifileVersion() != AVIFILE_VERSION)
    {
	fprintf(stderr, "This binary was compiled for Avifile ver. %d "
		", but the library is ver. %d. Aborting.\n",
		AVIFILE_VERSION, GetAvifileVersion());
	return 0;
    }

    if (argc == 0)
    {
	fprintf(stderr, "Missing argument: filename\n");
	exit(0);
    }

    try
    {
	if (!(f = fopen(outname, "wb")))
	    throw FATAL("Can't open %s for writing", outname);
	if (!(ac = avm::CreateReadFile(argv[1])))
	    throw FATAL("Can't read given file");
	if (!(as = ac->GetStream(0, avm::IStream::Audio)))
	    throw FATAL("Stream doesn't contain audio stream");

	zz = new uint8_t[buffer_size];
	while (!as->Eof())
	{
	    uint_t samp_read, bytes_read;
	    as->ReadDirect(zz, buffer_size, buffer_size, samp_read, bytes_read);
	    fwrite(zz, bytes_read, 1, f);
	}
    }
    catch (FatalError& error)
    {
	error.Print();
    }

    delete ac;
    delete zz;
    if (f)
	fclose(f);
}
