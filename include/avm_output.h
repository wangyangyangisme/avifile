#ifndef AVIFILE_AVM_OUTPUT_H
#define AVIFILE_AVM_OUTPUT_H

#ifdef __cplusplus
#include "avm_default.h"
#include "avm_locker.h"
#include "avm_stl.h"

#include <stdarg.h>

/**********
 * WARNING - this file is meant to be used by internal avifile application
 * DO NOT USE in your own project!
 * the API here could change in any minute
 */


AVM_BEGIN_NAMESPACE;

typedef void (*handlerFuncPtr) (const char* s, int opt);


class AvmOutput
{
public:
    AvmOutput() :priv(0) {};
    ~AvmOutput();
    void write(const char* mode, const char* format, ...); // debuglevel 0
    void write(const char* mode, int debuglevel, const char* format, ...); // any debuglevel
    void vwrite(const char* mode, const char* format, va_list va);
    void vwrite(const char* mode, int debuglevel, const char* format, va_list va);
    void setDebugLevel(const char* mode, int level);
    void resetDebugLevels(int level = 0);
private:
    void vwrite(const char* format, va_list va);
    void flush();

    struct AvmOutputPrivate;
    struct AvmOutputPrivate* priv;
};

extern AvmOutput out;

#define AVM_WRITE avm::out.write

AVM_END_NAMESPACE

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif
    /** C interface **/
    void avm_printf(const char* mode, const char* format, ...);
    void avm_dprintf(const char* mode, int debuglevel, const char* format, ...);

#ifdef __cplusplus
};
#endif

/*
 * new logging
 *
 * for now only for internal avifiles' usage
 * should be 4bit signed value probably
 * < 0 - always a serious warning message
 * -7  - will lead to program abort
 * > 0 - debug message
 * = 0 - informational
 */
#define AVML_DEBUG 1
#define AVML_DEBUG1 2
#define AVML_DEBUG2 3
#define AVML_DEBUG3 4
#define AVML_DEBUG4 5
#define AVML_DEBUG5 6
#define AVML_INFO 0
#define AVML_WARN -1
#define AVML_FATAL -7
/* not yet decided about the implementation */
#ifndef avml
#define avml(a, fmt, args...) \
	do { if (avm_debug_level >= a) fprintf(stderr, fmt, ## args); } while (0)
extern int avm_debug_level;
#endif

#endif
