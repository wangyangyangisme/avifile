#ifndef AVIFILE_MMX_H
#define AVIFILE_MMX_H

/********************************************************
 *
 *      Miscellaneous MMX-accelerated routines
 *      Copyright 2000 Eugene Kuznetsov (divx@euro.ru)
 *
 ********************************************************/

// generic scaling function for 16, 24 or 32 bit data
void zoom(uint16_t* dest, const uint16_t* src, int dst_w, int dst_h, int src_w, int src_h, int bpp, int xdim=0);

// converter from 555 to 565 color depth
extern void (*v555to565)(uint16_t* dest, const uint16_t* src, int w, int h);

// fast scaler by-2 ( e.g. 640x480->320x240 ) for 16-bit colors */
extern void (*zoom_2_16)(uint16_t* dest, const uint16_t* src, int w, int h);

// scaler & converter
extern void (*zoom_2_16_to565)(uint16_t *dest, const uint16_t* src, int w, int h);

// scaler by-2 for 32-bit colors
extern void (*zoom_2_32)(uint32_t* dest, const uint32_t* src, int w, int h);

#endif // AVIFILE_MMX_H
