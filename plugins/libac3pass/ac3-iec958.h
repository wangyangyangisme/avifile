#ifndef _AC3_IEC958_H
#define _AC3_IEC958_H

#define IEC61937_DATA_TYPE_AC3 1

#ifdef __cplusplus
extern "C"
{
#endif

struct ac3info {
  int bitrate, framesize, samplerate, bsmod;
};

void ac3_iec958_build_burst(unsigned int length, int data_type, int big_endian, unsigned char * data, unsigned char * out);
int ac3_iec958_parse_syncinfo(unsigned char *buf, int size, struct ac3info *ai, int *skipped);

#ifdef __cplusplus
}
#endif

#endif
