#ifndef AVIFILE_MJPEG_H
#define AVIFILE_MJPEG_H

#include <stdio.h>
#include <sys/types.h>

// it's stupid bug jpeglib is including it's own jconfig.h
#undef HAVE_STDLIB_H
#include <jpeglib.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
    unsigned int width;
    unsigned int height;
    int invert_direction;
    JOCTET* mjpg_buffer;
    size_t mjpg_bufsize;
    size_t mjpg_datasize;
    size_t mjpg_bufused;
    int mjpg_tables;
    int warn_shown;
    struct jpeg_error_mgr mjpg_jerr;
    struct jpeg_destination_mgr mjpg_dest;
    struct jpeg_source_mgr mjpg_src;
} mjpg_client;

struct jpeg_compress_struct* mjpg_bgr_init(int width, int height, int quality);
int  mjpg_bgr_compress(struct jpeg_compress_struct* mjpg_cinfo,
		       unsigned char* d, const unsigned char *s, int p);
//void mjpg_yuv_init(int width, int height);
//int  mjpg_yuv422_compress(unsigned char *d, unsigned char *s, int p);
//int  mjpg_yuv420_compress(unsigned char *d, unsigned char *s, int p);
void mjpg_cleanup(struct jpeg_compress_struct*);

void mjpg_dec_cleanup(struct jpeg_decompress_struct*);
struct jpeg_decompress_struct* mjpg_dec_bgr_init(int width, int height);
int mjpg_bgr_decompress(struct jpeg_decompress_struct* mjpg_dinfo,
			unsigned char *d, unsigned char *s, int p);

#if defined(__cplusplus)
}
#endif
#endif
