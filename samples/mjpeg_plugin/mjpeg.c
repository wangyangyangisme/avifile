#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mjpeg.h"
//#include "colorspace.h"

/* ---------------------------------------------------------------------- */

static int debug=0;

static inline void bgr2rgb(char* out, const char* in, unsigned int n)
{
    char* oute = out + 3 * n;

    while (out < oute)
    {
        char c = in[0];
	out[1] = in[1];
	out[0] = in[2];
	out[2] = c;

	c = in[3];
	out[4] = in[4];
	out[3] = in[5];
	out[5] = c;

	out += 6;
        in += 6;
    }
}

/* ---------------------------------------------------------------------- */

static void mjpg_dest_init(struct jpeg_compress_struct* cinfo)
{
    mjpg_client* mc = (mjpg_client*)cinfo->client_data;
    cinfo->dest->next_output_byte = mc->mjpg_buffer;
    cinfo->dest->free_in_buffer   = mc->mjpg_bufsize;
}

static boolean mjpg_dest_flush(struct jpeg_compress_struct* cinfo)
{
    fprintf(stderr,"mjpg: panic: output buffer too small\n");
    exit(1);
}

static void mjpg_dest_term(struct jpeg_compress_struct* cinfo)
{
    mjpg_client* mc = (mjpg_client*)cinfo->client_data;
    mc->mjpg_bufused = mc->mjpg_bufsize - cinfo->dest->free_in_buffer;
}

static void mjpg_src_init(struct jpeg_decompress_struct* dinfo)
{
    mjpg_client* mc = (mjpg_client*) dinfo->client_data;
    //printf("Init  %p  %d\n", mc->mjpg_buffer, mc->mjpg_datasize);
    dinfo->src->next_input_byte = mc->mjpg_buffer;
    dinfo->src->bytes_in_buffer = mc->mjpg_datasize;
}
static boolean mjpg_src_fill(struct jpeg_decompress_struct* dinfo)
{
    printf("Fill\n");
    return -1;
}
static void mjpg_src_skip(struct jpeg_decompress_struct* dinfo, long bytes)
{
    //printf("Skip  %ld\n", bytes);
    dinfo->src->next_input_byte += bytes;
    dinfo->src->bytes_in_buffer -= bytes;
}
static boolean mjpg_src_resync(struct jpeg_decompress_struct *dinfo, int desired)
{
    printf("Resync\n");
    return -1;
}
static void mjpg_src_term(struct jpeg_decompress_struct* dinfo)
{
    //printf("SrcTerm\n");
}

static void mjpg_init_client(mjpg_client* mc)
{
    memset(mc, 0, sizeof(mjpg_client));
    mc->mjpg_dest.empty_output_buffer = mjpg_dest_flush;
    mc->mjpg_dest.init_destination = mjpg_dest_init;
    mc->mjpg_dest.term_destination = mjpg_dest_term;

    mc->mjpg_src.fill_input_buffer = mjpg_src_fill;
    mc->mjpg_src.init_source       = mjpg_src_init;
    mc->mjpg_src.resync_to_restart = mjpg_src_resync;
    mc->mjpg_src.skip_input_data   = mjpg_src_skip;
    mc->mjpg_src.term_source       = mjpg_src_term;
    mc->mjpg_src.bytes_in_buffer   = 0;
    mc->mjpg_src.next_input_byte   = NULL;

    jpeg_std_error(&mc->mjpg_jerr);
}

/* ---------------------------------------------------------------------- */

static struct jpeg_compress_struct* mjpg_init(int width, int height)
{
    mjpg_client* mc;
    struct jpeg_compress_struct* mjpg_cinfo =
	malloc(sizeof(struct jpeg_compress_struct));

    if (!mjpg_cinfo)
	return 0;

    mc = malloc(sizeof(mjpg_client));
    if (!mc)
    {
	free(mjpg_cinfo);
        return 0;
    }

    memset(mjpg_cinfo, 0, sizeof(struct jpeg_compress_struct));
    mjpg_init_client(mc);
    mjpg_cinfo->client_data = mc;
    mjpg_cinfo->err = &mc->mjpg_jerr;

    jpeg_create_compress(mjpg_cinfo);
    mjpg_cinfo->dest = &mc->mjpg_dest;

    if (height < 0)
    {
	height *= -1;
	mc->invert_direction = 0;
    }
    else
	mc->invert_direction = 1;

    mjpg_cinfo->image_width  = width;
    mjpg_cinfo->image_height = height;
    mc->mjpg_tables = TRUE;

    return mjpg_cinfo;
}

static struct jpeg_decompress_struct* mjpg_dec_init(int width, int height)
{
    mjpg_client* mc;
    struct jpeg_decompress_struct* mjpg_dinfo =
	malloc(sizeof(struct jpeg_decompress_struct));
    if (!mjpg_dinfo)
	return 0;

    mc = malloc(sizeof(mjpg_client));
    if (!mc)
    {
	free(mjpg_dinfo);
        return 0;
    }
    memset(mjpg_dinfo, 0, sizeof(struct jpeg_decompress_struct));

    mjpg_init_client(mc);
    mjpg_dinfo->client_data = mc;
    mjpg_dinfo->err = &mc->mjpg_jerr;

    jpeg_create_decompress(mjpg_dinfo);
    mjpg_dinfo->src = &mc->mjpg_src;

    if (height < 0)
    {
	height *= -1;
	mc->invert_direction = 0;
    }
    else
	mc->invert_direction = 1;

    mc->width = width;
    mc->height = height;
    mc->mjpg_tables = TRUE;

    return mjpg_dinfo;
}

/* ---------------------------------------------------------------------- */

struct jpeg_compress_struct* mjpg_bgr_init(int width, int height, int quality)
{
    struct jpeg_compress_struct* mjpg_cinfo;
    if (debug > 1)
	fprintf(stderr,"mjpg_rgb_init\n");

    mjpg_cinfo = mjpg_init(width, height);
    if (mjpg_cinfo)
    {
	mjpg_cinfo->input_components = 3;
	mjpg_cinfo->in_color_space = JCS_RGB;

	jpeg_set_defaults(mjpg_cinfo);
	mjpg_cinfo->dct_method = JDCT_FASTEST;
	jpeg_set_quality(mjpg_cinfo, quality, TRUE);
	jpeg_suppress_tables(mjpg_cinfo, TRUE);
    }

    return mjpg_cinfo;
}

void mjpg_cleanup(struct jpeg_compress_struct* mjpg_cinfo)
{
    if (debug > 1)
	fprintf(stderr,"mjpg_cleanup\n");

    jpeg_destroy_compress(mjpg_cinfo);
    free(mjpg_cinfo->client_data);
    free(mjpg_cinfo);
}

/* ---------------------------------------------------------------------- */

struct jpeg_decompress_struct* mjpg_dec_bgr_init(int width, int height)
{
    struct jpeg_decompress_struct* mjpg_dinfo;
    if (debug > 1)
	fprintf(stderr,"mjpg_dec_rgb_init\n");

    mjpg_dinfo = mjpg_dec_init(width, height);

    if (mjpg_dinfo)
    {
	mjpg_dinfo->num_components = 3;
	mjpg_dinfo->jpeg_color_space = JCS_RGB;
	mjpg_dinfo->out_color_space = JCS_RGB;
	mjpg_dinfo->scale_num = 1;
	mjpg_dinfo->scale_denom = 1;
	mjpg_dinfo->output_gamma = 0;
	mjpg_dinfo->dct_method = JDCT_FASTEST;
	//jpeg_set_defaults(&mjpg_dinfo);
	//    mjpg_cinfo.dct_method = JDCT_FASTEST;
	//    jpeg_set_quality(&mjpg_cinfo, mjpeg_quality, TRUE);
	//    jpeg_suppress_tables(&mjpg_cinfo, TRUE);
    }

    return mjpg_dinfo;
}

void mjpg_dec_cleanup(struct jpeg_decompress_struct* mjpg_dinfo)
{
    if (debug > 1)
	fprintf(stderr,"mjpg_cleanup\n");

    jpeg_destroy_decompress(mjpg_dinfo);
    free(mjpg_dinfo->client_data);
    free(mjpg_dinfo);
}

int mjpg_bgr_compress(struct jpeg_compress_struct* mjpg_cinfo,
		      unsigned char *d, const unsigned char *s, int p)
{
    int i;
    const unsigned char *line;
    unsigned char* hb;
    mjpg_client* mc = (mjpg_client*) mjpg_cinfo->client_data;

    if (debug > 1)
	fprintf(stderr,"mjpg_rgb_compress\n");

    mc->mjpg_buffer  = d;
    mc->mjpg_bufsize = 3*mjpg_cinfo->image_width*mjpg_cinfo->image_height;
    hb = d + mc->mjpg_bufsize;
    jpeg_start_compress(mjpg_cinfo, mc->mjpg_tables);
    if (!mc->invert_direction)
	for (i = 0, line = s; i < mjpg_cinfo->image_height;
	     i++, line += 3*mjpg_cinfo->image_width)
	{
	    //bgr2rgb(hb, line, mjpg_cinfo->image_width);
	    //jpeg_write_scanlines(mjpg_cinfo, &hb, 1);
            memcpy(hb, line, mjpg_cinfo->image_width * 3);
	    jpeg_write_scanlines(mjpg_cinfo, &hb, 1);
	}
    else
	for (i = 0, line = s + mc->mjpg_bufsize-3*mjpg_cinfo->image_width;
	     i < mjpg_cinfo->image_height;
	     i++, line -= 3*mjpg_cinfo->image_width)
	{
	    bgr2rgb(hb, line, mjpg_cinfo->image_width);
	    jpeg_write_scanlines(mjpg_cinfo, &hb, 1);
	}

    jpeg_finish_compress(mjpg_cinfo);
    mc->mjpg_tables = FALSE;

    return mc->mjpg_bufused;
}

int mjpg_bgr_decompress(struct jpeg_decompress_struct* mjpg_dinfo,
			unsigned char *d, unsigned char *s, int size)
{
    int i;
    unsigned char *line;
    mjpg_client* mc = (mjpg_client*)mjpg_dinfo->client_data;
    static int ix = 0;
    char bx[100];
    FILE* fx;

#if 0
    sprintf(bx, "file%d.jpg", ix++);
    fx = fopen(bx, "w+");
    fwrite(s, size, 1, fx);
    fclose(fx);
#endif

    if (debug > 1)
	fprintf(stderr,"mjpg_rgb_decompress\n");

    mc->mjpg_datasize = size;
    mc->mjpg_buffer = s;

    if (jpeg_read_header(mjpg_dinfo, 1) != JPEG_HEADER_OK)
	return -1;
    if ((mc->width != mjpg_dinfo->image_width
	 || mc->height != mjpg_dinfo->image_height)
	&& !mc->warn_shown)
    {
	fprintf(stderr, "WARNING: incompatible headers! (AVI: %d x %d  JPEG: %d x %d)\n",
                mc->width, mc->height,
		mjpg_dinfo->image_width, mjpg_dinfo->image_height);
	mc->warn_shown++;
    }

    //fprintf(stderr,"mjpg_rgb_decompress1 %d   %d   %p  skip:%d\n", i, size, mc, mc->mjpg_skip);
    jpeg_start_decompress(mjpg_dinfo);
    if(!mc->invert_direction)
    {
	char buf[5000];
	for (i = 0, line = d; i < mc->height;
	     i++, line += 3 * mc->width)
	{
	    jpeg_read_scanlines(mjpg_dinfo, &line, 1);
	    bgr2rgb(line, line, mc->width);
	}
        line = buf;
	for (; i < mjpg_dinfo->image_height; i++)
	    jpeg_read_scanlines(mjpg_dinfo, &line, 1);
    }
    else
	for (i = 0, line = d + 3 * mc->width * (mc->height - 1);
	     i < mc->height;  i++, line -= 3 * mc->width)
	{
	    jpeg_read_scanlines(mjpg_dinfo, &line, 1);
	    bgr2rgb(line, line, mjpg_dinfo->image_width);
	}
    jpeg_finish_decompress(mjpg_dinfo);
    mc->mjpg_tables = FALSE;

    return 0;
}

/* ---------------------------------------------------------------------- */
#if 0
static int rwidth,rheight;
unsigned char **mjpg_ptrs[3];
unsigned char **mjpg_run[3];

void mjpg_yuv_init(int width, int height)
{
    if (debug > 1)
	fprintf(stderr,"mjpg_yuv_init\n");

    // save real size
    rwidth  = width;
    rheight = height;

    // fix size to match DCT blocks (I'm not going to copy around
    //   data to pad stuff, so we'll simplay cut off edges)
    width  &= ~(2*DCTSIZE-1);
    height &= ~(2*DCTSIZE-1);
    mjpg_init(width, height);

    mjpg_cinfo.input_components = 3;
    mjpg_cinfo.in_color_space = JCS_YCbCr;

    jpeg_set_defaults(&mjpg_cinfo);
    mjpg_cinfo.dct_method = JDCT_FASTEST;
    jpeg_set_quality(&mjpg_cinfo, mjpeg_quality, TRUE);

    mjpg_cinfo.raw_data_in = TRUE;
    jpeg_set_colorspace(&mjpg_cinfo,JCS_YCbCr);
    mjpg_cinfo.comp_info[0].h_samp_factor = 2;
    mjpg_cinfo.comp_info[0].v_samp_factor = 2;
    mjpg_cinfo.comp_info[1].h_samp_factor = 1;
    mjpg_cinfo.comp_info[1].v_samp_factor = 1;
    mjpg_cinfo.comp_info[2].h_samp_factor = 1;
    mjpg_cinfo.comp_info[2].v_samp_factor = 1;

    mjpg_ptrs[0] = malloc(height*sizeof(char*));
    mjpg_ptrs[1] = malloc(height*sizeof(char*)/2);
    mjpg_ptrs[2] = malloc(height*sizeof(char*)/2);
    jpeg_suppress_tables(&mjpg_cinfo, TRUE);
}


static int mjpg_yuv_compress(void)
{
    int y;

    mjpg_run[0] = mjpg_ptrs[0];
    mjpg_run[1] = mjpg_ptrs[1];
    mjpg_run[2] = mjpg_ptrs[2];

    //    mjpg_cinfo.write_JFIF_header = FALSE;
    jpeg_start_compress(&mjpg_cinfo, mjpg_tables);
    //    jpeg_write_marker(&mjpg_cinfo, JPEG_APP0, "AVI1\0\0\0\0", 8);
    for (y = 0; y < mjpg_cinfo.image_height; y += 2*DCTSIZE) {
	jpeg_write_raw_data(&mjpg_cinfo, mjpg_run,2*DCTSIZE);
	mjpg_run[0] += 2*DCTSIZE;
	mjpg_run[1] += DCTSIZE;
	mjpg_run[2] += DCTSIZE;
    }
    jpeg_finish_compress(&(mjpg_cinfo));
    //    mjpg_tables = FALSE;

    return mjpg_bufused;
}

int mjpg_yuv422_compress(unsigned char *d, unsigned char *s, int p)
{
    unsigned char *line;
    int i;

    if (debug > 1)
	fprintf(stderr,"mjpg_yuv422_compress\n");

    mjpg_buffer  = d;
    mjpg_bufsize = 3*mjpg_cinfo.image_width*mjpg_cinfo.image_height;

    line = s;
    for (i = 0; i < mjpg_cinfo.image_height; i++, line += rwidth)
	mjpg_ptrs[0][i] = line;

    line = s + rwidth*rheight;
    for (i = 0; i < mjpg_cinfo.image_height; i+=2, line += rwidth)
	mjpg_ptrs[1][i/2] = line;

    line = s + rwidth*rheight*3/2;
    for (i = 0; i < mjpg_cinfo.image_height; i+=2, line += rwidth)
	mjpg_ptrs[2][i/2] = line;

    return mjpg_yuv_compress();
}

int mjpg_yuv420_compress(unsigned char *d, unsigned char *s, int p)
{
    unsigned char *line;
    int i;

    if (debug > 1)
	fprintf(stderr,"mjpg_yuv420_compress\n");

    mjpg_buffer  = d;
    mjpg_bufsize = 3*mjpg_cinfo.image_width*mjpg_cinfo.image_height;

    line = s;
    for (i = 0; i < mjpg_cinfo.image_height; i++, line += rwidth)
	mjpg_ptrs[0][i] = line;

    line = s + rwidth*rheight;
    for (i = 0; i < mjpg_cinfo.image_height; i+=2, line += rwidth/2)
	mjpg_ptrs[1][i/2] = line;

    line = s + rwidth*rheight*5/4;
    for (i = 0; i < mjpg_cinfo.image_height; i+=2, line += rwidth/2)
	mjpg_ptrs[2][i/2] = line;

    return mjpg_yuv_compress();
}
#endif
