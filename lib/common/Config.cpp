#include "Config.h"

#include "configfile.h"
#include "avm_output.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

AVM_BEGIN_NAMESPACE;

static int compare_entry(const void* e1, const void* e2)
{
    const ConfigEntry* ce1 = (const ConfigEntry*) e1;
    const ConfigEntry* ce2 = (const ConfigEntry*) e2;
    int r = strcmp(ce1->appname.c_str(), ce2->appname.c_str());
    if (r == 0)
	r = strcmp(ce1->valname.c_str(), ce2->valname.c_str());
    return r;
}


ConfigFile::ConfigFile(const char* fn) : dirty(false), opened(false)
{
    Open(fn);
}

ConfigFile::~ConfigFile()
{
    Close();
}

ConfigEntry* ConfigFile::Find(const char* appname, const char* valname)
{
    if (opened)
    {
	for (unsigned i = 0; i < entries.size(); i++)
	{
/*
	    printf("FIND %d:%d   %s %s       %s %s    \n",
                   i, entries.size(),
		   (const char*)appname, (const char*)valname,
		   (const char*)entries[i].appname,
		   (const char*)entries[i].valname);
*/
	    if (entries[i].appname == appname
		&& entries[i].valname == valname)
		return &entries[i];
	}
    }
    return 0;
}

void ConfigFile::Open(const char* fn)
{
    if (opened)
	if (filename == fn)
	    return;
	else
	    Close();

    FILE* f = fopen(fn, "rb");
    filename = fn;
    opened = true;
    dirty = false;
    if(!f)
	return;

    char appname[256], buf[4096], type;

    while (fgets(buf, sizeof(buf), f))
    {
	char* bp = buf;
	buf[sizeof(buf) - 1] = 0;
	while (isspace(*bp)) bp++;

	if (!*bp)
            continue;
	if (*bp == '[')
	{
	    sscanf(++bp, "%s", appname);
            continue;
	}

	char* bpt = bp;
	while (!isspace(*bp) && *bp) bp++;
        if (*bp)
	    *bp++ = 0;
	while (isspace(*bp)) bp++;

	ConfigEntry e(appname, bpt);

	bpt = bp;
	while (*bp != '\n' && *bp) bp++;
        *bp = 0;
	e.value = bpt;

	if (e.valname.size() > 0)
	{
	    e.type = ConfigEntry::Binary;
	    entries.push_back(e);
	}
    }
    fclose(f);
}

void ConfigFile::Save()
{

    if (!opened || !dirty)
	return;

    qsort(entries.begin(), entries.size(), sizeof(ConfigEntry), compare_entry);
    // FIXME - do backup copy before overwrite!  (rename)
    //printf("PATHNAME %s<>%s<\n", pathname, getConfigPath());
    FILE* f = fopen(filename.c_str(), "wb");
    if (!f)
    {
	AVM_WRITE("Config", "WARNING: can't save configuration %s\n", strerror(errno));
	return;
    }

    avm::string last;
    for (unsigned i=0; i<entries.size(); i++)
    {
	if (entries[i].appname != last)
	{
	    fprintf(f, "\n[ %s ]\n", entries[i].appname.c_str());
            last = entries[i].appname;
	}

	fprintf(f, "%s ", entries[i].valname.c_str());
	switch(entries[i].type)
	{
	case ConfigEntry::Int:
	    fprintf(f, "%d\n", entries[i].i);
	    break;
	case ConfigEntry::Float:
	    fprintf(f, "%f\n", entries[i].f);
	    break;
	case ConfigEntry::Binary:
	    if (entries[i].value.size() > 0)
		fwrite(entries[i].value.c_str(), entries[i].value.size(), 1, f);
	    fprintf(f, "\n");
	    break;
	}
    }

    fclose(f);


}
void ConfigFile::Close()
{
    if (!opened || !dirty)
	return;

    Save();

    opened = false;
}

void ConfigFile::push_back(const ConfigEntry& e)
{
    dirty = true;
    entries.push_back(e);
}

ConfigEntry::ConfigEntry(const char* _appname, const char* _valname)
    : type(Int), i(0)
{
    if (_appname)
	appname = _appname;
    if (_valname)
        valname = _valname;
}

ConfigEntry::ConfigEntry(const ConfigEntry& e)
    : type(Int), i(0)
{
    operator=(e);
}

ConfigEntry::~ConfigEntry()
{
}

ConfigEntry& ConfigEntry::operator=(const ConfigEntry& e)
{
    type = e.type;
    appname = e.appname;
    valname = e.valname;
    value.erase();
    switch (type)
    {
    case Binary:
	value = e.value;
	break;
    case Int:
	i = e.i;
        break;
    case Float:
	f = e.f;
	break;
    }
    return *this;
}

AVM_END_NAMESPACE;
