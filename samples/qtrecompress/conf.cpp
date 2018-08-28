#include "conf.h"
#include <avm_except.h>

#include <stdio.h>
#include <string.h>
#include <fstream>

#define __MODULE__ "Read config"

using namespace avm;

ReadConfig::ReadConfig(const char* fn)
{
    FILE *is = fopen(fn, "r");
    string s;
    if (!is)
	throw FATAL("Could not open file");
    while (!feof(is))
    {
	char z[500];
	z[499]=0;
	//is.gets(&z);
	if (!fgets(z, 499, is))
	{
	    fprintf(stderr,"Z=0!\n");
	    break;
	}
	int endpos=strlen(z)-1;
	while((endpos>=0) && (z[endpos]=='\n'))
	{
	    z[endpos]=0;
	    endpos--;
	}
	s=string(z);
//	cerr<<s<<endl;
	avm::string::size_type pos=s.find(':');
	if(pos==avm::string::npos)
	{
//	    cerr<<"NO : !"<<endl;
	    break;
	}
	avm::string key=s;
	key.erase(pos);
	string value=s;
	value.erase(0, pos+1);
	_map.insert(key, value);
	fprintf(stderr,"Key: %s  Value: %s\n", key.c_str(), value.c_str());
	//delete z;
    }
}

double ReadConfig::getDouble(const avm::string& key) const
{
    return atof(_map.find_default(key)->c_str());
}
int ReadConfig::getInt(const avm::string& key) const
{
    return strtol(_map.find_default(key)->c_str(), 0, 16);
}
const char* ReadConfig::getString(const avm::string& key) const
{
    return *(_map.find_default(key));
}

#undef __MODULE__
#define __MODULE__ "Write config"
WriteConfig::WriteConfig(const char* fn)
{
    fh = fopen(fn, "w");
    if (!fh)
	throw FATAL("Could not open file");
}

WriteConfig::~WriteConfig()
{
    avm::avm_map<avm::string, avm::string>::const_iterator it;
    for (it = _map.begin(); it != _map.end(); it++)
	if (it->key != "")
	    fprintf((FILE*)fh, "%s:%s", it->key.c_str(), it->value.c_str());
    fclose((FILE*)fh);
}

void WriteConfig::add(const avm::string& key, const avm::string& value)
{
    _map.insert(key, value);
}

void WriteConfig::add(const avm::string& key, int value)
{
    char s[64];
    sprintf(s, "%d", value);
    _map.insert(key, string(s));
}

void WriteConfig::add(const avm::string& key, double value)
{
    char s[128];
    sprintf(s, "%f", value);
    _map.insert(key, string(s));
}
