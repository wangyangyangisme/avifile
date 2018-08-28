#include "avm_except.h"
#include "utils.h"
#include "avm_output.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h> //vsnprintf
#include <stdlib.h> //free

BaseError::~BaseError()
{
    if (module) free(module);
    if (description) free(description);
    if (severity) free(severity);
}

BaseError::BaseError()
    :file(0), module(0), description(0), severity(0), line(0)
{
}

BaseError::BaseError(const char* sev, const char* mod, const char* f, int l, const char* desc)
    :file(f), line(l)
{
    module = (char*) malloc(strlen(mod) + 1);
    if (!module) abort();
    strcpy(module, mod);
    description = (char*) malloc(strlen(desc) + 256);
    if (!description) abort();
    description[0] = 0;
    severity = (char*) malloc(strlen(sev) + 1);
    if (!severity) abort();
    strcpy(severity, sev);
}

BaseError::BaseError(const BaseError& f)
    :module(0), description(0), severity(0)
{
    operator=(f);
}

BaseError& BaseError::operator=(const BaseError& f)
{
    file = f.file;
    line = f.line;
    if (module) free(module);
    if (description) free(description);
    if (severity) free(severity);

    module = (char*) malloc(strlen(f.module) + 1);
    if (!module) abort();
    strcpy(module, f.module);
    description = (char*) malloc(strlen(f.description) + 1);
    if (!description) abort();
    strcpy(description, f.description);
    severity = (char*) malloc(strlen(f.severity) + 1);
    if (!severity) abort();
    strcpy(severity, f.severity);

    return *this;
}

void BaseError::Print()
{
    AVM_WRITE("exception", "%s: %s: %s\n", module, severity, description);
}

void BaseError::PrintAll()
{
    char bf[256];
    int p = 0;
    if (file && strlen(file) < 230)
	p = sprintf(bf, " at %s", file);
    if (line) 
	p += sprintf(bf + p, ": %d", line);
    AVM_WRITE("exception", "%s: %s: %s%s\n",
	      module, severity, description, bf);
}

const char* BaseError::GetModule() const
{
    return module;
}

const char* BaseError::GetDesc() const
{
    return description;
}

FatalError::FatalError(const char* mod, const char* f, int l, const char* desc,...)
    :BaseError("FATAL", mod, f, l, desc)
{
    va_list va;
    va_start(va, desc);
    vsnprintf(description, strlen(desc)+255, desc, va);
    va_end(va);
}

GenError::GenError(const char* mod, const char* f, int l, const char* desc,...)
    :BaseError("WARNING", mod, f, l, desc)
{
    va_list va;
    va_start(va, desc);
    vsnprintf(description, strlen(desc)+255, desc, va);
    va_end(va);
}
