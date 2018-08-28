#ifndef DSP_H
#define DSP_H

class dsp
{
public:
    dsp();
    ~dsp();

    int open(int bits, int channels, int rate, const char* dev = "/dev/dsp");
    int close();
    int getBufSize() { return blocksize; }
    int getSize();
    int read(char* buffer, int size);
    int synch();
private:
    dsp(const dsp & );

    int fd, blocksize;
    char *buffer;
};

#endif // DSP_H
