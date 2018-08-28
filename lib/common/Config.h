#ifndef AVIFILE_CONFIG_H
#define AVIFILE_CONFIG_H

#include "avm_stl.h"

AVM_BEGIN_NAMESPACE;

/**********
 * WARNING - this file is meant to be used by internal avifile application
 * DO NOT USE in your own project!
 * the API here could change in any minute
 */

struct ConfigEntry
{
    enum Types
    {
	Int,
	Float,
	Binary
    };
    Types type;
    avm::string appname;
    avm::string valname;
    avm::string value;
    union
    {
	int i;
	float f;
    };

    ConfigEntry(const char* appname = 0, const char* valname = 0);
    ~ConfigEntry();
    ConfigEntry(const ConfigEntry& e);
    ConfigEntry& operator= (const ConfigEntry& e);

    void SetFloat(float _f) { value.erase(); f = _f; type = Float; }
    void SetInt(int _i) { value.erase(); i = _i; type = Int; }
    void SetString(const char* _value) { value = _value; type = Binary; }
};

struct ConfigFile
{
    avm::string filename;
    avm::vector<ConfigEntry> entries;
    bool dirty;
    bool opened;

    ConfigFile(const char* fn);
    ~ConfigFile();
    void Close();
    void Save(); 
    void Open(const char* fn);
    ConfigEntry *Find(const char* appname, const char* valname);
    void push_back(const ConfigEntry& e);

    void sort();
};

AVM_END_NAMESPACE;

#endif // AVIFILE_CONFIG_H
