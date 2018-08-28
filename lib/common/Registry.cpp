#include "configfile.h"
#include "Config.h"
#include "avm_output.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h> // atexit
#include <stdio.h>
#include <pwd.h>

AVM_BEGIN_NAMESPACE

/* avoiding using static objects - using just pointers */
static char* sConfigDir = 0;
static char* sConfigName = 0;
static ConfigFile* config = 0;

// called at exit
static void destroy_config()
{
    if (sConfigDir) free(sConfigDir);
    if (sConfigName) free(sConfigName);
    delete config;
    sConfigDir = 0;
    sConfigName = 0;
    config = 0;
}

static ConfigFile* get_config()
{
    if (!config)
    {
	char* home;
	home = getenv("HOME");
	if (home == 0)
	{
	    struct passwd* pwent = getpwuid(getuid());
	    home = pwent->pw_dir;
	}
	avm::string s(home);
	if (!sConfigDir)
	    s += "/.avm";
	else
	{
	    s += "/";
	    s += sConfigDir;
	}

        struct stat st;
	if (stat(s.c_str(), &st))
	{
	    AVM_WRITE("Registry", "creating dir: %s\n", s.c_str());
            mkdir(s.c_str(), 0755);
	};

	if (!sConfigName)
	    s += "/default";
	else
	{
	    s += "/";
	    s += sConfigName;
	}

	config = new ConfigFile(s.c_str());
	atexit(destroy_config);
    }

    return config;
}

void* RegInit(const char* regname, const char* dirname)
{
    destroy_config();
    sConfigDir = strdup(dirname);
    sConfigName = strdup(regname);

    return get_config();
}

void RegSave(){
  get_config()->Save();
}

int RegWriteInt(const char* appname, const char* valname, int value)
{
    ConfigEntry* e = get_config()->Find(appname, valname);
    if (e)
    {
	e->SetInt(value);
	config->dirty = true;
    }
    else
    {
	//printf("WRITEINT NOT FOUND %s  %s  %d\n", appname.c_str(), valname.c_str(), value);
	ConfigEntry en(appname, valname);
	en.SetInt(value);
	config->push_back(en);
    }
    return 0;
}

int RegReadInt(const char* appname, const char* valname, int def_value)
{
    ConfigEntry* e = get_config()->Find(appname, valname);
    if (e)
    {
	if (e->type != ConfigEntry::Int)
	{
	    if (e->type != ConfigEntry::Binary
		|| (sscanf(e->value.c_str(), "%d", &e->i) != 1))
		return -1;
	    e->type = ConfigEntry::Int;
	}
	return e->i;
    }
    RegWriteInt(appname, valname, def_value);
    return def_value;
}

int RegWriteFloat(const char* appname, const char* valname, float value)
{
    ConfigEntry* e = get_config()->Find(appname, valname);
    if(e)
    {
	e->SetFloat(value);
	config->dirty = true;
    }
    else
    {
	ConfigEntry en(appname, valname);
	en.SetFloat(value);
	config->push_back(en);
    }
    return 0;
}

float RegReadFloat(const char* appname, const char* valname, float def_value)
{
    ConfigEntry* e = get_config()->Find(appname, valname);
    if (e)
    {
	if (e->type != ConfigEntry::Float)
	{
	    if (e->type != ConfigEntry::Binary
		|| (sscanf(e->value.c_str(), "%f", &e->f) != 1))
		return -1;
            e->type = ConfigEntry::Float;
	}
	return e->f;
    }
    RegWriteFloat(appname, valname, def_value);
    return def_value;
}

int RegWriteString(const char* appname, const char* valname, const char* value)
{
    ConfigEntry* e = get_config()->Find(appname, valname);
    if (e)
    {
	e->SetString(value);
	config->dirty = true;
    }
    else
    {
	ConfigEntry en(appname, valname);
	en.SetString(value);
	config->push_back(en);
    }
    return 0;
}

const char* RegReadString(const char* appname, const char* valname, const char* def_value)
{
    ConfigEntry* e = get_config()->Find(appname, valname);

    if (e)
    {
	if (e->type != ConfigEntry::Binary)
	    return "";
	return e->value.c_str();
    }

    RegWriteString(appname, valname, def_value);
    return def_value;
}

AVM_END_NAMESPACE
