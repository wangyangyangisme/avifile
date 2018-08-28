#include "image.h"
#include "avm_fourcc.h"
#include "avm_output.h"
#include <stdlib.h> // labs
#include <string.h>
//#include <stdio.h>

AVM_BEGIN_NAMESPACE;

BitmapInfo::BitmapInfo()
{
}

BitmapInfo::BitmapInfo(int width, int height, int bpp)
{
    memset(this, 0, sizeof(BitmapInfo));
    biWidth = width;
    biHeight = height;
    biPlanes = 1;
    switch (bpp)
    {
    case 8:
    case 15:
    case 16:
    case 24:
    case 32:
	SetBits(bpp);
	break;
    default:
	SetSpace(bpp);
	break;
    }
}

BitmapInfo::BitmapInfo(const BITMAPINFOHEADER& hdr)
{
    unsigned int c = hdr.biSize;
    if (c > sizeof(BitmapInfo))
    {
	AVM_WRITE("BitmapInfo", "BitmapInfo::Unknown format (BIH) - size: %d\n", hdr.biSize);
	c = sizeof(BitmapInfo);
    }

    memcpy(this, &hdr, c);
    if (c <= sizeof(BITMAPINFOHEADER))
    {
	m_iColors[0] = m_iColors[1] = m_iColors[2] = 0;
    }
}

BitmapInfo::BitmapInfo(const BitmapInfo& bi)
{
    if (bi.biSize > int(sizeof(BitmapInfo)))
	AVM_WRITE("BitmapInfo", "BitmapInfo::Unknown format (BI) - size: %d\n", bi.biSize);

    memcpy(this, &bi, sizeof(BitmapInfo));
}

BitmapInfo::BitmapInfo(const BitmapInfo* bi)
{
    if (!bi)
	AVM_WRITE("BitmapInfo", "BitmapInfo::Invalid argument\n");
    else
    {
	if (bi->biSize > int(sizeof(BitmapInfo)))
	    AVM_WRITE("BitmapInfo", "BitmapInfo::Unknown format (BI*) - size: %d\n", bi->biSize);
	memcpy(this, bi, sizeof(BitmapInfo));
    }
}

int BitmapInfo::BitCount(int csp)
{
    switch (csp)
    {
    case IMG_FMT_YUV:
    case IMG_FMT_I444:
	return 24;
    case IMG_FMT_I422:
    case IMG_FMT_Y422:
    case IMG_FMT_YUY2:
    case IMG_FMT_UYVY:
    case IMG_FMT_YVYU:
	return 16;
    case IMG_FMT_YV12:
    case IMG_FMT_IYUV:
    case IMG_FMT_I420:
    case IMG_FMT_I411:
	return 12;
    case IMG_FMT_Y800:
	return 8;
    }
    return 0;
}

int BitmapInfo::Bpp() const
{
    if (biCompression == BI_RGB && biBitCount == 16)
	return 15; // 16bit BI_RGB is 5-5-5

    if (biCompression == BI_BITFIELDS && m_iColors[0] == 0x7c00)
	return 15;

    return biBitCount;
}


void BitmapInfo::SetBitFields16()
{
    biBitCount = 16;
    SetRGB();
    biSize = sizeof(BitmapInfo);
    biCompression = BI_BITFIELDS;
    m_iColors[0] = 0xF800;
    m_iColors[1] = 0x07E0;
    m_iColors[2] = 0x001F;
}

void BitmapInfo::SetBitFields15()
{
    biBitCount = 16;
    SetRGB();
    biSize = sizeof(BitmapInfo);
    biCompression = BI_BITFIELDS;
    m_iColors[0] = 0x7C00;
    m_iColors[1] = 0x03E0;
    m_iColors[2] = 0x001F;
}

void BitmapInfo::SetRGB()
{
    biSize = sizeof(BITMAPINFOHEADER);
    biCompression = BI_RGB;
    biPlanes = 1;
    biSizeImage = biWidth * labs(biHeight) * ((biBitCount + 7) / 8);
    biHeight = -labs(biHeight);
}

void BitmapInfo::SetBits(int bits)
{
    switch (bits)
    {
    case 16:
	SetBitFields16();
	break;
    case 15:
	bits = 16; // this is legal and probably prefered way by M$
	// we will use BI_RGB format instead of BI_BITFIELD
        // this format says it's 16 bit - but it is in reality 15 bit
	//SetBitFields15();
	//break;
    default:
	biBitCount = bits;
	SetRGB();
	break;
    }
}

void BitmapInfo::SetSpace(int csp)
{
    biSize = sizeof(BITMAPINFOHEADER);
    biCompression = csp;
    biPlanes = 1;
    biBitCount = BitCount(csp);
    biHeight = -labs(biHeight);
    biSizeImage = biWidth * labs(biHeight) * biBitCount / 8;
}

void BitmapInfo::Print() const
{
    AVM_WRITE("BitmapInfo", 0, "BitmapInfo, format: \n");
    AVM_WRITE("BitmapInfo", 0, "  biSize %d ( %d x %d x %db ) %d bytes\n",
	      biSize, biWidth, biHeight, biBitCount, biSizeImage);
    AVM_WRITE("BitmapInfo", 0, "  biPlanes %d,  biCompression 0x%08x='%.4s'\n",
	      biPlanes, biCompression, (const char *)&biCompression);
    if (biSize > 40)
	AVM_WRITE("BitmapInfo", 0, "  colors:  0x%04x  0x%04x  0x%04x\n",
		  m_iColors[0], m_iColors[1], m_iColors[2]);
}

AVM_END_NAMESPACE;
