#include "avm_cpuinfo.h"
#include "avm_creators.h"
#include "avm_output.h"
#include "infotypes.h"
#include "utils.h"
#include "plugin.h"
#include "version.h"
#include <string.h>
#include <stdlib.h> //atof
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif
#include <time.h>
#ifdef	HAVE_KSTAT
#include <kstat.h>
#endif
#include <ctype.h>
#include <stdio.h>

AVM_BEGIN_NAMESPACE;

BaseInfo::BaseInfo()
{
}

BaseInfo::BaseInfo(const char* info, const char* _about)
    : name(info), about(_about)
{
}

BaseInfo::~BaseInfo()
{
}

AttributeInfo::AttributeInfo(const char* n, const char* a, const char** o, int defitem)
    : BaseInfo(n, a), kind(Select), i_min(0), i_max(-1), i_default(defitem)
{
    while (*o)
    {
	options.push_back(*o);
	o++;
    }
    i_max = options.size();
}

AttributeInfo::AttributeInfo()
{
}

AttributeInfo::AttributeInfo(const char* n, const char* a, Kind k, int minval, int maxval, int defval)
    : BaseInfo(n, (a) ? a : ""), kind(k), i_min(minval), i_max(maxval), i_default(defval)
{
    if (i_default == -1)
        i_default = (i_min + i_max) / 2;
}

AttributeInfo::AttributeInfo(const char* n, const char* a, float defval, float minval, float maxval)
    : BaseInfo(n, (a) ? a : ""), kind(Float), f_min(minval), f_max(maxval), f_default(defval)
{
}

AttributeInfo::~AttributeInfo()
{
}

bool AttributeInfo::IsAttr(const char* attribute) const
{
    return (strcmp(GetName(), attribute) == 0
	   || strcmp(GetAbout(), attribute) == 0);
}

CodecInfo::CodecInfo() :handle(0) //, codec(0)
{
}

CodecInfo::CodecInfo(const fourcc_t* array, const char* info, const char* path, const char* a,
		     Kind _kind, const char* pn, Media _media, Direction _direction, const GUID* id,
		     const avm::vector<AttributeInfo>& ei,
		     const avm::vector<AttributeInfo>& di)
    : BaseInfo(info, a), fourcc(array[0]), privatename(pn), kind(_kind),
    media(_media), direction(_direction), dll(path),
    decoder_info(di), encoder_info(ei),
    handle(0)//, codec(0),
{
    if (id)
	guid = *id;

    if (!*array) // uncompressed codec
    {
	fourcc_array.push_back(0);
	array++;
    }

    while (*array)
    {
	fourcc_array.push_back(*array);
	array++;
    }
}

CodecInfo::CodecInfo(const CodecInfo& ci) :handle(0)
{
    operator=(ci);
}

CodecInfo::~CodecInfo()
{
    if (handle)
    {
	//((PluginPrivate*)handle)->refcount--;
	//printf("HAND-\n");
    }
}

CodecInfo& CodecInfo::operator=(const CodecInfo& ci)
{
    name = ci.name;
    about = ci.about;
    handle = ci.handle;

    if (handle)
    {
	//((PluginPrivate*)handle)->refcount++;
        //printf("HAND+\n");
    }

    assert(handle == 0);
#if 0
    if (ci.handle)
    {
	(plugin_internal*) handle = new plugin_internal;
	((plugin_internal*)handle)->dlhandle = ((plugin_internal*)ci.handle)->dlhandle;
	((plugin_internal*)handle)->plhandle = ((plugin_internal*)ci.handle)->plhandle;
    }
    else
	handle = 0;
    codec = ci.codec;
#endif
    fourcc = ci.fourcc;
    fourcc_array = ci.fourcc_array;
    kind = ci.kind;
    media = ci.media;
    direction = ci.direction;
    decoder_info = ci.decoder_info;
    encoder_info = ci.encoder_info;
    modulename = ci.modulename;
    guid = ci.guid;
    dll = ci.dll;
    privatename = ci.privatename;
    return *this;
}

const AttributeInfo* CodecInfo::FindAttribute(const char* attr, Direction dir) const
{
    if (attr)
    {
	if (dir == Both || dir == Encode)
	{
	    avm::vector<AttributeInfo>::const_iterator it;
	    for (it = encoder_info.begin(); it != encoder_info.end(); it++)
		if (it->IsAttr(attr))
		    return &(*it);
	}
	if (dir == Both || dir == Decode)
	{
	    avm::vector<AttributeInfo>::const_iterator it;
	    for (it = decoder_info.begin(); it != decoder_info.end(); it++)
		if (it->IsAttr(attr))
		    return &(*it);
	}
    }
    return 0;
}

// CodecInfo::match is in codeckeeper.cpp - it's using Create functions

#ifdef WIN32
#define rdtsc __asm _emit 0fh __asm _emit 031h
#define cpuid __asm _emit 0fh __asm _emit 0a2h

#define strncasecmp strnicmp
#endif

static void do_cpuid(uint_t *regs, int op)
{
    unsigned int ax, cx;
    *regs = 0;
#ifdef ARCH_X86

    /* first check if cpuid is supported - ffmpeg */
    __asm__ __volatile__
	(
	 /* See if CPUID instruction is supported ... */
	 /* ... Get copies of EFLAGS into eax and ecx */
	 "pushf\n\t"
	 "popl %0\n\t"
	 "movl %0, %1\n\t"

	 /* ... Toggle the ID bit in one copy and store */
	 /*     to the EFLAGS reg */
	 "xorl $0x200000, %0\n\t"
	 "push %0\n\t"
	 "popf\n\t"

	 /* ... Get the (hopefully modified) EFLAGS */
	 "pushf\n\t"
	 "popl %0\n\t"
	 : "=a" (ax), "=c" (cx)
	 :
	 : "cc"
	);

    if (ax == cx)
	return; /* CPUID not supported */

    ax = op;
    __asm__ __volatile__
	(
	 "pushl %%ebx; pushl %%ecx; pushl %%edx;"
	 ".byte  0x0f, 0xa2;"
	 "movl   %%eax, (%2);"
	 "movl   %%ebx, 4(%2);"
	 "movl   %%ecx, 8(%2);"
	 "movl   %%edx, 12(%2);"
	 "popl %%edx; popl %%ecx; popl %%ebx;"
	 : "=a" (ax)
	 :  "0" (ax), "S" (regs));
#endif
    return;
}

static uint_t localcount_tsc()
{
    int a;
#ifndef WIN32
#ifdef ARCH_X86
    __asm__ __volatile__("rdtsc\n\t" : "=a"(a) : : "edx");
#else
    a = 0;
#endif
#else
    __asm
    {
	rdtsc
    	mov a, eax
    }
#endif
    return a;
}

static int64_t longcount_tsc()
{
#ifndef WIN32
#ifdef ARCH_X86
    int64_t l;
    __asm__ __volatile__("rdtsc\n\t":"=A"(l));
    return l;
#else
    return 0;
#endif
#else
    unsigned long i;
    unsigned long j;
    __asm
    {
	rdtsc
	mov i, eax
	mov j, edx
    }
    return ((int64_t)j<<32)+(int64_t)i;
#endif
}

static uint_t localcount_notsc()
{
#ifndef WIN32
    static const uint32_t limit= ~0U / 1000000;
    struct timeval tv;
    gettimeofday(&tv, 0);
    return limit*tv.tv_usec;
#else
    return time(NULL);
#endif
}

static int64_t longcount_notsc()
{
#ifndef WIN32
    static const uint32_t limit= ~0U / 1000000;
    struct timeval tv;
    gettimeofday(&tv, 0);
    uint64_t result;
    result=tv.tv_sec;
    result<<=32;
    result+=limit*tv.tv_usec;
    return result;
#else
    return time(NULL);
#endif
}

static double old_freq()
{
#if HAVE_KSTAT
    /*
     * try to extact the CPU speed from the Solaris kernel's kstat data
     */
    kstat_ctl_t   *kc;
    kstat_t       *ksp;
    kstat_named_t *kdata;
    int            mhz = 0;

    kc = kstat_open();
    if (kc != NULL)
    {
	ksp = kstat_lookup(kc, "cpu_info", 0, "cpu_info0");

	/* kstat found and name/value pairs? */
	if (ksp != NULL && ksp->ks_type == KSTAT_TYPE_NAMED)
	{
	    /* read the kstat data from the kernel */
	    if (kstat_read(kc, ksp, NULL) != -1)
	    {
		/*
		 * lookup desired "clock_MHz" entry, check the expected
		 * data type
		 */
		kdata = (kstat_named_t *)kstat_data_lookup(ksp, "clock_MHz");
		if (kdata != NULL && kdata->data_type == KSTAT_DATA_INT32)
		    mhz = kdata->value.i32;
	    }
	}
	kstat_close(kc);
    }

    if (mhz > 0)
	return mhz * 1000.;
#endif	/* HAVE_KSTAT */

    int i=time(NULL);
    int64_t x,y;
    while(i==time(NULL));
    x=longcount();
    i++;
    while(i==time(NULL));
    y=longcount();
    return (double)(y-x)/1000.;
}

void CPU_Info::Init()

{
    char line[200];
    char model[200]="unknown";
    char flags[500]="";
    char* s, *value;
    bool is_AMD=false;

    freq = -1;
    have_tsc = have_mmx = have_mmxext = have_sse = false;

    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f)
    {
	uint_t regs[4];
	do_cpuid(regs, 0);
	is_AMD = ((regs[1] == 0x68747541) &&
	         (regs[2] == 0x444d4163) &&
    	         (regs[3] == 0x69746e65));

	do_cpuid(regs, 1);
	have_tsc=regs[3] & 0x00000010;
	have_mmx=regs[3] & 0x00800000;
	have_mmxext=regs[3] & 0x02000000;
	have_sse=regs[3] & 0x02000000;
	do_cpuid(regs, 0x80000000);
	if (regs[0] >= 0x80000001)
	{
	    do_cpuid(regs, 0x80000001);

	    if(is_AMD && (regs[3] & 0x00400000))
		have_mmxext=1;
	}

	if (have_tsc)
	{
	    longcount = longcount_tsc;
	    localcount = localcount_tsc;
	}
	else
	{
	    longcount = longcount_notsc;
	    localcount = localcount_notsc;
	}

	freq = old_freq();
	return;
    }

    while (fgets(line, 200, f) != NULL)
    {
	/* NOTE: the ':' is the only character we can rely on */
	value = strchr(line,':');
	if (!value)
	    continue;
	/* terminate the valuename */
	*value++ = '\0';
	/* skip any leading spaces */
	while (*value == ' ')
	    value++;

        s = strchr(value, '\n');
	if (s)
	    *s = '\0';

	if (!strncasecmp(line, "cpu MHz",strlen("cpu MHz")))
	{
	    freq=atof(value);
	    freq*=1000;
	}
	if (!strncasecmp(line, "model name", strlen("model name")))
	    strncpy(model, value, sizeof(model)-1);
	if (!strncasecmp(line, "flags", strlen("flags")) ||
	    !strncasecmp(line, "features", strlen("features")))
	    strncpy(flags, value, sizeof(flags)-1);
	continue;
    }
    fclose(f);

    AVM_WRITE("init", 0, "Avifile %s\n", AVIFILE_BUILD);
    AVM_WRITE("init", 0, "Available CPU flags: %s\n", flags);
    have_tsc = (strstr(flags, "tsc") != 0);
    have_mmx = (strstr(flags, "mmx") != 0);
    have_sse = (strstr(flags, "sse") != 0);
    have_mmxext = (strstr(flags, "mmxext") != 0) | have_sse;

    if (0 && have_tsc) // disabled - notebooks doesn't work!
    {
	longcount = longcount_tsc;
	localcount = localcount_tsc;
    }
    else
    {
        freq = ~0U/(double)1000;
	longcount = longcount_notsc;
	localcount = localcount_notsc;
    }

    if (freq < 0)
	freq = old_freq();

    if (have_tsc)
	AVM_WRITE("init", 0, "%.2f MHz %s %sdetected\n", freq/1000., model,
		  strstr(model, "rocessor") ? "" : "processor ");
}

AVM_END_NAMESPACE;
