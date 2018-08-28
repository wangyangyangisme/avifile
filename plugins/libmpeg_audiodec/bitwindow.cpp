/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Bitwindow.cc
// It's bit reservior for MPEG layer 3

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mpegsound.h"

AVM_BEGIN_NAMESPACE;

int Mpegbitwindow::getbits(int bits)
{
  int current;
  int bi;

  if(!bits)return 0;

  current=0;
  bi=(bitindex&7);

  current = buffer[bitindex>>3]<<bi & 0xff;
  bi=8-bi;
  bitindex+=bi;

  while(bits)
  {
    if(!bi)
    {
      current |= buffer[bitindex>>3] & 0xff;
      bitindex+=8;
      bi=8;
    }

    if(bits>=bi)
    {
      current<<=bi;
      bits-=bi;
      bi=0;
    }
    else
    {
      current<<=bits;
      bi-=bits;
      bits=0;
    }
  }
  bitindex-=bi;

  return (current>>8);
}

AVM_END_NAMESPACE;
