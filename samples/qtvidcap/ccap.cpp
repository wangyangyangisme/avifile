/**
 * NTSC closed caption decoder
 * Based on cc.c from xawtv distribution, written by Mike Baker (mbm@linux.com)
 * (based on code by timecop@japan.co.jp)
**/
#include "ccap.h"

#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "v4lxif.h"

const char* const ClosedCaption::specialchar[] = {"®","°","½","¿","(TM)","¢","£","o/~ ","à"," ","è","â","ê","î","ô","û"};
const int ClosedCaption::rowdata[] = {11,-1,1,2,3,4,12,13,14,15,5,6,7,8,9,10};
const char* const ClosedCaption::ratings[] = {"(NOT RATED)","TV-Y","TV-Y7","TV-G","TV-PG","TV-14","TV-MA","(NOT RATED)"};
const char* const ClosedCaption::modes[]={"current","future","channel","miscellaneous","public service","reserved","invalid","invalid","invalid","invalid"};


static inline int parityok(int n)	/* check parity for 2 bytes packed in n */
{
    int mask=0;
    int j, k;
    for (k = 1, j = 0; j < 7; j++)
    {
	  if (n & (1<<j))
	    k++;
    }
    if ((k & 1) == ((n>>7)&1))
	  mask|=0x00FF;
    for (k = 1, j = 8; j < 15; j++)
    {
	if (n & (1<<j))
    	    k++;
    }
    if ((k & 1) == ((n>>15)&1))
	    mask|=0xFF00;
   return mask;
}

static inline int decodebit(unsigned char *data, int threshold)
{
    int i, sum = 0;
    for (i = 0; i < 23; i++)
	  sum += data[i];
    return (sum > threshold*23);
}

int ClosedCaption::decode(unsigned char *vbiline)
{
    int max[7], min[7], val[7], i, clk, tmp, sample, packedbits = 0;

    for (clk=0; clk<7; clk++)
	  max[clk] = min[clk] = val[clk] = -1;
    clk = tmp = 0;
    i=30;

    while (i < 600 && clk < 7)
    {	/* find and lock all 7 clocks */
	sample = vbiline[i];
	if (max[clk] < 0)
	{ /* find maximum value before drop */
	    if (sample > 85 && sample > val[clk])
		(val[clk] = sample, tmp = i);	/* mark new maximum found */
	    else if (val[clk] - sample > 30)	/* far enough */
		(max[clk] = tmp, i = tmp + 10);
	}
	else
        { /* find minimum value after drop */
    	    if (sample < 85 && sample < val[clk])
		(val[clk] = sample, tmp = i);	/* mark new minimum found */
	    else if (sample - val[clk] > 30)	/* searched far enough */
		(min[clk++] = tmp, i = tmp + 10);
	}
	i++;
    }

    i=min[6]=min[5]-max[5]+max[6];

    if (clk != 7 || vbiline[max[3]] - vbiline[min[5]] < 45)		/* failure to locate clock lead-in */
	return -1;

    /* calculate threshold */
    for (i=0,sample=0;i<7;i++)
	    sample=(sample + vbiline[min[i]] + vbiline[max[i]])/3;

    for(i=min[6];vbiline[i]<sample;i++);

    tmp = i+57;
    for (i = 0; i < 16; i++)
	if(decodebit(&vbiline[tmp + i * 57], sample))
	    packedbits |= 1<<i;
    return packedbits&parityok(packedbits);
} /* decode */

int webtv_check(char * buf,int len)
{
	unsigned long   sum;
	unsigned long   nwords;
	unsigned short  csum=0;
	char temp[9];
	int nbytes=0;

	while (buf[0]!='<' && len > 6)  //search for the start
	{
		buf++; len--;
	}

	if (len == 6) //failure to find start
		return 0;


	while (nbytes+6 <= len)
	{
		//look for end of object checksum, it's enclosed in []'s and there shouldn't be any [' after
		if (buf[nbytes] == '[' && buf[nbytes+5] == ']' && buf[nbytes+6] != '[')
			break;
		else
			nbytes++;
	}
	if (nbytes+6>len) //failure to find end
		return 0;

	nwords = nbytes >> 1; sum = 0;

	//add up all two byte words
	while (nwords-- > 0) {
		sum += *buf++ << 8;
		sum += *buf++;
	}
	if (nbytes & 1) {
		sum += *buf << 8;
	}
	csum = (unsigned short)(sum >> 16);
	while(csum !=0) {
		sum = csum + (sum & 0xffff);
		csum = (unsigned short)(sum >> 16);
	}
	sprintf(temp,"%04X\n",(int)~sum&0xffff);
	buf++;
	if(!strncmp(buf,temp,4))
	{
		buf[5]=0;
		printf("\33[35mWEBTV: %s\33[0m\n",buf-nbytes-1);
		fflush(stdout);
	}
	return 0;
}

int ClosedCaption::CCdecode(int data)
{
    int b1, b2, row, len, x,y;
    if (data == -1) //invalid data. flush buffers to be safe.
    {
    	memset(ccbuf[1],0,255);
    	memset(ccbuf[2],0,255);
    	return -1;
    }
    b1 = data & 0x7f;
    b2 = (data>>8) & 0x7f;
    len = strlen(ccbuf[ccmode]);

    if (b1&0x60 && data != lastcode) // text
    {
    	ccbuf[ccmode][len++]=b1;
	if (b2&0x60) ccbuf[ccmode][len++]=b2;
	if (b1 == ']' || b2 == ']')
	    webtv_check(ccbuf[ccmode],len);
    }
    else if ((b1&0x10) && (b2>0x1F) && (data != lastcode)) //codes are always transmitted twice (apparently not, ignore the second occurance)
    {
	ccmode=((b1>>3)&1)+1;
	len = strlen(ccbuf[ccmode]);

	if (b2 & 0x40)	//preamble address code (row & indent)
	{
	    row=rowdata[((b1<<1)&14)|((b2>>5)&1)];
	    if (len!=0)
		ccbuf[ccmode][len++]='\n';

	    if (b2&0x10) //row contains indent flag
		for (x=0;x<(b2&0x0F)<<1;x++)
		    ccbuf[ccmode][len++]=' ';
	}
	else
	{
	    switch (b1 & 0x07)
	    {
	    case 0x00:	//attribute
		printf("<ATTRIBUTE %d %d>\n",b1,b2);
		fflush(stdout);
		break;
	    case 0x01:	//midrow or char
		switch (b2&0x70)
		{
		case 0x20: //midrow attribute change
		    switch (b2&0x0e)
		    {
		    case 0x00: //italics off
			if (!plain)
			//    strcat(ccbuf[ccmode],"\33[0m ");
			    strcat(ccbuf[ccmode],"</i> ");
			break;
		    case 0x0e: //italics on
			if (!plain)
			//    strcat(ccbuf[ccmode],"\33[36m ");
			    strcat(ccbuf[ccmode],"<i> ");
			break;

		    }
		    if (b2&0x01)
		    { //underline
		    	if (!plain)
			//    strcat(ccbuf[ccmode],"\33[4m");
			    strcat(ccbuf[ccmode],"</u>");
		    }
		    else
		    {
			if (!plain)
			    strcat(ccbuf[ccmode],"<u>");
//			    strcat(ccbuf[ccmode],"\33[24m");
		    }
		    break;
		case 0x30: //special character..
		    strcat(ccbuf[ccmode],specialchar[b2&0x0f]);
		    break;
		}
		break;
		case 0x04:	//misc
		case 0x05:	//misc + F
//			printf("ccmode %d cmd %02x\n",ccmode,b2);
		switch (b2)
		{
		    case 0x21: //backspace
		    ccbuf[ccmode][len--]=0;
			break;

		/* these codes are insignifigant if we're ignoring positioning */
		    case 0x25: //2 row caption
		    case 0x26: //3 row caption
		    case 0x27: //4 row caption
		    case 0x29: //resume direct caption
		    case 0x2B: //resume text display
		    case 0x2C: //erase displayed memory
			break;

		    case 0x2D: //carriage return
			if (ccmode==2)
			    break;
		    case 0x2F: //end caption + swap memory
		    case 0x20: //resume caption (new caption)
			if (!strlen(ccbuf[ccmode]))
			    break;
/*
		        for (x=0;x<strlen(ccbuf[ccmode]);x++)
			    for (y=0;y<keywords;y++)
				if (!strncasecmp(keyword[y], ccbuf[ccmode]+x, strlen(keyword[y])))
					printf("\a");
*/
//			printf("%s\33[m\n",ccbuf[ccmode]);
			lock();
			strcpy(storage, ccbuf[ccmode]);
			unlock();
			for(unsigned i=0; i<m_callbacks.size(); i++)
			    m_callbacks[i].exec(storage);
//			fflush(stdout);
			/* FALL */
		    case 0x2A: //text restart
		    case 0x2E: //erase non-displayed memory
			memset(ccbuf[ccmode],0,255);
			break;
		}
		break;
	    case 0x07:	//misc (TAB)
		for(x=0;x<(b2&0x03);x++)
	    	    ccbuf[ccmode][len++]=' ';
		break;
	    }
	}
    }
    lastcode=data;
    return 0;
}

//ClosedCaption::ClosedCaption(const char* pcDevName="/dev/vbi")
ClosedCaption::ClosedCaption(v4lxif* pv4l)
    : m_iRefcnt(1), m_pV4l(pv4l), m_pThread(0),
    m_iQuit(0), ccmode(1), plain(0)
{
//    m_iDev=open(pcDevName, O_RDONLY);
    m_iDev=m_pV4l->getvbidev();
    storage[0]=0;
    if (m_iDev>=0)
	m_pThread = new avm::PthreadTask(0, threadstarter, (void*)this);
}

void* ClosedCaption::threadstarter(void* pArg)
{
    ClosedCaption* pObject = (ClosedCaption*)pArg;
    return pObject->mainloop();
}

ClosedCaption::~ClosedCaption()
{
    if (m_iDev>=0)
    {
	m_iQuit = 1;
	delete m_pThread;
//	close(m_iDev);
    }
}

void* ClosedCaption::mainloop()
{
    fd_set rfds;
    unsigned char buf[65536];

    while (!m_iQuit)
    {
	timeval tv;
	tv.tv_sec=5;
	tv.tv_usec=0;
	FD_ZERO(&rfds);
	FD_SET(m_iDev, &rfds);
	select(m_iDev+1, &rfds, NULL, NULL, &tv);
	if (FD_ISSET(m_iDev, &rfds))
	{
	    if (m_pV4l->readvbi(buf , 65536)!=65536)
		printf("read error\n");
	    CCdecode(decode(&buf[2048 * 11]));
	}
	else
	{
	    lock();
	    storage[0]=0;
	    unlock();
	}
    }
    return 0;
}

void ClosedCaption::add_callback(callbackproc p, void* arg)
{
     callback_info ci;
     ci.proc=p;
     ci.arg=arg;
     m_callbacks.push_back(ci);
}

void ClosedCaption::remove_callback(callbackproc p, void* arg)
{
     for(avm::vector<callback_info>::iterator it=m_callbacks.begin(); it!=m_callbacks.end(); it++)
     {
        if(it->proc!=p)
	    continue;
	if(it->arg!=arg)
	    continue;
        m_callbacks.erase(it);
	return;
    }
}
