#ifndef AVM_ARGS_H
#define AVM_ARGS_H

#include "avm_default.h"

AVM_BEGIN_NAMESPACE;

class Args
{
public:
    struct Option {
	enum Type {
            NONE,
	    HELP,   		// use default help text and options
	    CODEC,   		// show available codec options (char*)
	    OPTIONS,		// value is pointer to another Options array
				// this options are separated by ':'
	    SUBOPTIONS,		// value is pointer to another Options array
				// this options are separated by ':'
	    BOOL,       	// true flag when option is present
	    DOUBLE,        	// double from given range (max == min -> unlimited)
	    INT,        	// integer from given range (max == min -> unlimited)
	    STRING,  		// any string could be given
	    SELECTSTRING,     	// string from the list

	    REG = 128,           // marks usage of register value
            REGBOOL =   REG | BOOL,
            REGDOUBLE = REG | DOUBLE,
            REGINT =    REG | INT,
            REGSTRING = REG | STRING,

	    ARRAY = 256		// array marks usage of register value
	} type;
	const char* oshort;	// short option name (NULL - no short opt)
	const char* olong;	// long option name (NULL - no long opt)
	const char* help;	// help text (%d  %s)

	void* value;      	// storage pointer for value
				// for SUB/OPTIONS contains subarray with Options
                                // for REG| contains pointer to default value
	int min;		// min integer value
	int max;		// max integer value
	const char* defstr;	// list of string options
    };

    Args(const Option* options, int* argc, char** argv,
	 const char* help = 0, const char* regname = 0);
    ~Args();
    // could be used to parse CodecInfo arguments
    static void ParseCodecInfo(const char* str);
protected:
    int findOpt(int olong = 0);

    const Option* opts;
    int* argc;
    char** argv;
    const char* help;
    const char* regname;
    int idx;
};

AVM_END_NAMESPACE;

#endif /* AVM_ARGS_H */
