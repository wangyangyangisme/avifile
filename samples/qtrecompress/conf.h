#ifndef CONF_H
#define CONF_H

#include "avm_stl.h"
#include "avm_map.h"

#include <stdlib.h>

class ReadConfig
{
    avm::avm_map<avm::string, avm::string> _map;
public:
    ReadConfig(const char* fn);
    ~ReadConfig(){}
    double getDouble(const avm::string& key) const;
    int getInt(const avm::string& key) const;
    const char* getString(const avm::string& key) const;
};

class WriteConfig
{
    avm::avm_map<avm::string, avm::string> _map;
    void* fh;
public:
    WriteConfig(const char* fn);
    ~WriteConfig();
    void add(const avm::string& key, const avm::string& value);
    void add(const avm::string& key, int value);
    void add(const avm::string& key, double value);
};

#endif // CONF_H
