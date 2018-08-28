/*
    Various more-or-less accurate examples.
*/    

#include <videodecoder.h>
#include <aviplay.h>
#include <avifile.h>
#include <avm_creators.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

extern int ASF_DEBUG;
//extern int DSHOW_DEBUG;
#define __MODULE__ "test"

using namespace std;

int main(int argc, char** argv)
{
    avm::IWriteFile* avi_file = avm::CreateWriteFile("test.avi");
    WAVEFORMATEX wfm;
    wfm.wFormatTag=WAVE_FORMAT_PCM;
    wfm.nChannels=1;                
    wfm.nSamplesPerSec=44100*1;     // frequency x channels
    wfm.nAvgBytesPerSec=2*44100;
    wfm.nBlockAlign=2;      // 2 byte per sample ? what value should I give here ?
    wfm.wBitsPerSample=16;
    wfm.cbSize=0;

    avm::IAudioWriteStream *stream;
// ( the object "avi_file" has already been created elsewhere)
    stream=avi_file->AddAudioStream(WAVE_FORMAT_MPEGLAYER3, &wfm, 8000);    //mp3, 8000ko/s (128kbits/s)
    stream->Start();     
    char tmp[100];     
//    stream->AddData(tmp, 6);
//    stream->Stop();
//    delete avi_file;
    // Load Win32 DLL and get address of one of its exports.
//    HMODULE lib=LoadLibrary("/usr/lib/win32/acelpdec.ax");
//    HMODULE lib=LoadLibrary("/usr/lib/win32/ir50_32.dll");
//    void* addr=(void*)GetProcAddress(lib, "DllGetClassObject");


    // Open WMV file, read some data ( with implicit decompression )
    // from its streams.
//    DSHOW_DEBUG=1;
//    LOADER_DEBUG=1;
//    ASF_DEBUG=1;    
//    IAviReadFile* file=CreateIAviReadFile("/d/test-movies/kelssyV_500VBR.wmv");
//    IAviReadFile* file=CreateIAviReadFile("/d/test-movies/iv5.avi");
//    IAviReadFile* file=CreateIAviReadFile("/d/movies/Eyes on me.asf");
//    IAviReadFile* file=CreateIAviReadFile("/d/movies/Baby one more time.asf");
//    IAviReadFile* file=CreateIAviReadFile("/d/movies/mission2mars.avi");
//    IAviReadFile* file=CreateIAviReadFile("/d/test-movies/dest2.avi");
//    IAviReadFile* file=CreateIAviReadFile("/d/movies/Lucky - Britney Spears.avi");
//    IAviReadFile* file=CreateIAviReadFile("/d/test-movies/smth.asf");
//    IAviReadFile* file=CreateIAviReadFile("/d/movies/Windows Movie Maker Sample File.wmv");
/*
    const char* s="/d/test-movies/i263.avi";
    if(argc>1)s=argv[1];
    IAviReadFile* file=CreateIAviReadFile(s);
    IAviReadStream* ss=file->GetStream(0, AviStream::Audio);
    char qwe[100000];
    ss->StartStreaming();
//    ss->Seek((unsigned int)12);
    unsigned int samp_read, b_read;
    const int size=19530;
    ss->ReadFrames(qwe, size, size, samp_read, b_read);
    printf("%d/%d\n", samp_read, b_read);
    
    IAviReadStream* stream=file->GetStream(0, AviStream::Video);
    stream->StartStreaming();
//    stream->GetDecoder()->SetDestFmt(16);
//    stream->StopStreaming();
//    stream->StartStreaming();

//    unsigned int prev_kf=stream->GetPrevKeyFrame(240);
//    printf("prev_kf: %d\n", prev_kf);
//    stream->Seek(prev_kf);
//    while(stream->GetPos()<240 && !stream->Eof())
//	stream->ReadFrame();

    stream->ReadFrame();
//    IRtConfig* cf=dynamic_cast<IRtConfig*>(stream->GetDecoder());
//    if(cf)cf->SetValue("Brightness", 123);
//    if(cf)cf->SetValue("Contrast", -78);
    stream->ReadFrame();
    stream->ReadFrame();
    stream->ReadFrame();
    stream->ReadFrame();
    stream->ReadFrame();
    stream->ReadFrame();
    stream->ReadFrame();
    stream->ReadFrame();
    CImage* zz=stream->GetFrame();
    unsigned char* qq=(unsigned char*)(zz->data());
    qq+=10*zz->bpl();
    printf("%02x %02x %02x %02x %02x %02x\n",
    qq[0], qq[1], qq[2], qq[3], qq[4], qq[5], qq[6]);
    zz->release();
    
    delete file;

*/
// Create an AVI player object for MMS URL and play it for some time.
//    char* fn="mms://208.184.229.156/vidnet0200/pp/bspears2pp.asf";
// mms://netshow.warnerbros.com/asfroot2/wbmovies/plutonash/teaser_56.wmv
// mms://208.184.229.153/launch/video/music/000/000/248/248651.asf
/*
    char* fn="mms://netshow.mp.intervu.net/rfvir_guiltypleasures_56";
    if(argc>1)fn=argv[1];    
    Display* _dpy=XOpenDisplay(0);//0 returned here is just fine
    IAviPlayer2* player=CreateAviPlayer2(0, _dpy, fn, 0);
    while(!player->IsOpened())sleep(1);
    if(!player->IsValid())
    {
	printf("Failed to open\n");
	delete player;
    }
    else
    {
	player->Start();
	sleep(2);
//	player->reseek(120.);
        sleep(1500);
	delete player;
    }
    if(_dpy)XCloseDisplay(_dpy);
  */ 
/*    

    // Decompress a compressed image.
    int fd=open("./compr.zzz", O_RDONLY);
    char header[0x28];
    read(fd, header, 0x28);
    IVideoDecoder* dc=CreateVideoDecoder(*(BITMAPINFOHEADER*)(&header[0]));
    if(!dc)throw FATAL(GetError().c_str());
//    dc->SetDestFmt(0, fccYUY2);
    dc->Start();
    char* pix=new char[100000];
    int size=read(fd, pix, 100000);
    dc->DecodeFrame(pix, size, 16);
    CImage* im=dc->GetFrame();
    close(fd);
    fd=open("./decompr.raw", O_WRONLY | O_CREAT);
    write(fd, im->data(), im->bytes());
    close(fd);
    dc->Stop();
    delete dc;

    return 0;
    */
}
