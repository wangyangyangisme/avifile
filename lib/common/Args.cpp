#include "avm_args.h"
#include "plugin.h"
#include "configfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

AVM_BEGIN_NAMESPACE;

static void split(avm::vector<avm::string>& arr, const char* a)
{
    if (a)
    {
	char *cp = strdup(a);
	if (cp)
	{
            char* c = cp;
	    char* b;
	    while ((b = strchr(c, ':')))
	    {
		*b++ = 0;
		if (strlen(c))
		    arr.push_back(c);
		c = b;
	    }
	    if (strlen(c))
		arr.push_back(c);
	    free(cp);
	}
	for (unsigned i = 0; i < arr.size(); i++)
	    printf("ARR %d  %s\n", i, arr[i].c_str());
    }
}

static void show_attrs(const CodecInfo& ci, const avm::vector<AttributeInfo>& attr, const char* title)
{
    if (attr.size())
    {
        int val;
	printf("    %s:\n", title);
	for (unsigned i = 0; i < attr.size(); i++)
	{
	    printf("      %20s", attr[i].GetName());
	    switch (attr[i].GetKind())
	    {
	    case AttributeInfo::Integer:
                PluginGetAttrInt(ci, attr[i].GetName(), &val);
		printf(" %d  default: %d  <%d, %d>", val,
		       attr[i].GetDefault(), attr[i].GetMin(), attr[i].GetMax());
		break;
	    case AttributeInfo::Select:
		PluginGetAttrInt(ci, attr[i].GetName(), &val);
		printf(" %s  default: %s <", attr[i].GetOptions()[val].c_str(),
		       attr[i].GetOptions()[attr[i].GetDefault()].c_str());
		for (int j = attr[i].GetMin(); j < attr[i].GetMax(); j++)
		{
		    if (j != attr[i].GetMin())
                        fputs(", ", stdout);
		    fputs(attr[i].GetOptions()[j].c_str(), stdout);
		}
		fputc('>', stdout);
                break;
	    default:
                ;
	    }
	    fputc('\n', stdout);
	}
    }
    else
        printf("    No %s\n", title);
}

static void set_codec_defaults(const CodecInfo& ci, const avm::vector<AttributeInfo>& attr)
{
    int val;
    for (unsigned i = 0; i < attr.size(); i++)
    {
	switch (attr[i].GetKind())
	{
	case AttributeInfo::Integer:
	case AttributeInfo::Select:
	    PluginSetAttrInt(ci, attr[i].GetName(), attr[i].GetDefault());
	default:
	    break;
	}
    }
}

static bool read_bool(const Args::Option* o, const char* arg, const char* par, const char* r)
{
    bool b = true;
    bool rs = false;

    if (par)
    {
	if (!strcasecmp(par, "off") || !strcmp(par, "0")
	    || !strcasecmp(par, "false"))
	{
	    b = false;
	    rs = true;
	}
	else if (!strcasecmp(par, "on") || !strcmp(par, "1")
		 || !strcasecmp(par, "true"))
	{
	    rs = true;
	}
    }

    if (o->type == Args::Option::REGBOOL)
	RegWriteInt(r, o->olong, b ? 1 : 0);
    else if (o->value)
	*(bool*)o->value = b;
    return rs;
}

static void read_double(const Args::Option* o, const char* arg, const char* par, const char* r)
{
    if (!par)
    {
	printf("Option: %s  - missing float value\n", arg);
	exit(1);
    }
    double d = atof(par);
    if (o->min != o->max)
    {
	if (d < o->min && d > o->max)
	{
	    printf("Option: %s  - value: %f  out of range <%d, %d>",
		   arg, d, o->min, o->max);
	    exit(1);
	}
    }
    if (o->type == Args::Option::REGDOUBLE)
	RegWriteFloat(r, o->olong, d);
    else if (o->value)
	*(double*)o->value = d;
}

static void read_int(const Args::Option* o, const char* arg, const char* par, const char* r)
{
    if (!par)
    {
	printf("Option: %s  - missing integer value\n", arg);
	exit(1);
    }
    int v = 0;
    sscanf(par, "%i", &v);
    //printf("READINT %s  %s  %d\n", arg, par, v);
    if (o->min != o->max)
    {
	if (v < o->min && v > o->max)
	{
	    printf("Option: %s  - value: %d  out of range <%d, %d>",
		   arg, v, o->min, o->max);
	    exit(1);
	}
    }
    if (o->type == Args::Option::REGINT)
	RegWriteInt(r, o->olong, v);
    else if (o->value)
	*(int*)o->value = v;
}

static void read_string(const Args::Option* o, const char* arg, const char* par, const char* r)
{
    if (!par)
    {
	printf("Option: %s  - missing string value\n", arg);
	exit(1);
    }
    if (o->type == Args::Option::REGSTRING)
	RegWriteString(r, o->olong, par);
    else if (o->value)
	*(char**)o->value = strdup(par);
}

static void show_help(const Args::Option* o, bool prefix)
{
    unsigned max = 0;
    avm::vector<avm::string> l;
    for (unsigned i = 0; o[i].type != Args::Option::NONE; i++)
    {
	char b[80];
	if (o[i].type == Args::Option::HELP)
	    sprintf(b, "  -h  --help");
	else
	    sprintf(b, "  %c%s  %s%s",
		    (o[i].oshort && prefix) ? '-' : ' ',
		    o[i].oshort ? o[i].oshort : " ",
		    (o[i].olong && prefix) ? "--" : "",
		    o[i].olong ? o[i].olong : ""
		    //opt[i].options ? opt[i].options : "",
		    //o[i].help ? o[i].help : ""
		   );
	l.push_back(avm::string(b));
	unsigned len = l.back().size();
	if (max < len)
	    max = len;
    }
    for (unsigned i = 0; o[i].type != Args::Option::NONE; i++)
    {
	if (!o[i].oshort && !o[i].olong
	    && o[i].type != Args::Option::HELP
	    && o[i].type != Args::Option::OPTIONS)
            continue;

	if (o[i].type != Args::Option::OPTIONS)
	{
	    fputs(l[i].c_str(), stdout);
	    for (unsigned s = l[i].size(); s <= max; s++)
		fputc(' ', stdout);
	    if (o[i].type == Args::Option::HELP)
		fputs("this help message", stdout);
	}

	if (o[i].value)
	{
	    switch (o[i].type)
	    {
	    case Args::Option::INT:
	    case Args::Option::REGINT:
                if (o[i].help)
		    printf(o[i].help, *(int*)o[i].value,
			   o[i].min, o[i].max);
		break;
	    case Args::Option::STRING:
	    case Args::Option::REGSTRING:
	    case Args::Option::SELECTSTRING:
                if (o[i].help)
		    printf(o[i].help, *(const char**)o[i].value);
		break;
	    case Args::Option::OPTIONS:
		show_help((const Args::Option*)o[i].value, prefix);
		continue;
	    default:
                if (o[i].help)
		    fputs(o[i].help, stdout);
		break;
	    }
	}
	else if (o[i].help)
	    fputs(o[i].help, stdout);

	fputs("\n", stdout);
    }
}

static void parse_suboptions(const Args::Option* o, const char* oname, const char* pars, const char* r)
{
    avm::vector<avm::string> arr;
    split(arr, pars);

    if (!arr.size() || strcmp(arr[0], "help") == 0)
    {
	printf("Available options for '%s' (optA=x:optB=...)\n", oname);
	show_help(o, false);
	exit(0);
    }
    for (unsigned i = 0; i < arr.size(); i++)
    {
	char* par = strchr(arr[i], '=');
	if (par)
	{
	    *par = 0;
	    par++;
	}

	for (unsigned j = 0; o[j].type != Args::Option::NONE; j++)
	{
	    if ((o[j].oshort && strcmp(o[j].oshort, arr[i]) == 0)
		|| (o[j].olong && strcmp(o[j].olong, arr[i]) == 0))
	    {
		switch(o[j].type)
		{
		case Args::Option::BOOL:
		    read_bool(&o[j], arr[i], par, r);
		    break;
		case Args::Option::DOUBLE:
		    read_double(&o[j], arr[i], par, r);
		    break;
		case Args::Option::INT:
		    read_int(&o[j], arr[i], par, r);
		    break;
		case Args::Option::STRING:
		    read_string(&o[j], arr[i], par, r);
		    break;
		default:
		    ;
		}
	    }
	}
    }
}

static void parse_codec(const Args::Option* o, const char* a)
{
    avm::vector<const CodecInfo*> ci;
    CodecInfo::Get(ci, CodecInfo::Video, CodecInfo::Both);
    CodecInfo::Get(ci, CodecInfo::Audio, CodecInfo::Both);

    avm::vector<avm::string> arr;
    split(arr, a);

    if (!arr.size() || arr[0] == "help")
    {
	const char* nm[] = { "audio", "video" };
	const char* nd[] = { 0, "encoder", "decoder", "de/encoder" };
	fputs("Available codecs:\n"
	      "Idx      Short name  Long name\n", stdout);
	for (unsigned i = 0; i < ci.size(); i++)
	    printf("%3d %15s  %s  (%s %s)\n", i + 1, ci[i]->GetPrivateName(),
		   ci[i]->GetName(), nm[ci[i]->media], nd[ci[i]->direction]);
        exit(0);
    }

    for (unsigned i = 0; i < ci.size(); i++)
    {
	const char* cname = ci[i]->GetPrivateName();
	if (arr[0] == cname)
	{
	    if (arr[1] == "help")
	    {
		printf("  Options for %s:\n", cname);
		show_attrs(*ci[i], ci[i]->decoder_info, "Decoding Options");
		show_attrs(*ci[i], ci[i]->encoder_info, "Encoding Options");
		exit(0);
	    }
	    else if (strcmp(arr[1], "defaults") == 0)
	    {
		set_codec_defaults(*ci[i], ci[i]->decoder_info);
		set_codec_defaults(*ci[i], ci[i]->encoder_info);
	    }
	    else
	    {
		for (unsigned j = 1; j < arr.size(); j++)
		{
		    char* p = strchr(arr[j].c_str(), '=');
		    int val = 0;
		    bool valid = false;
		    if (p)
		    {
			*p++ = 0;
			if (sscanf(p, "%i", &val) > 0)
			    valid = true;
		    }
		    const AttributeInfo* ai = ci[i]->FindAttribute(arr[j].c_str());
		    if (ai)
		    {
			switch (ai->GetKind())
			{
			case AttributeInfo::Integer:
			    if (!valid)
			    {
				printf("  Option %s for %s needs integer value! (given: %s)\n",
				       arr[j].c_str(), cname, p);
				exit(1);
			    }
			    printf("Setting %s = %d\n", arr[j].c_str(), val);
			    PluginSetAttrInt(*ci[i], arr[j].c_str(), val);
			    break;
			case AttributeInfo::Select:
			default:
			    ;
			}
		    }
		    else
		    {
			printf("  Unknown attribute name '%s' for '%s'\n",
			       arr[j].c_str(), cname);
			exit(1);
		    }
		}
	    }
	    if (o && o->value)
		*(char**)o->value = strdup(cname);
	    break;
	}
    }
    //for (unsigned i = 0; i < arr.size(); i++) printf("ARG %d   %s\n", i, arr[i].c_str());
}

void Args::ParseCodecInfo(const char* str)
{
    parse_codec(0, str);
}

Args::Args(const Option* _o, int* _argc, char** _argv,
	   const char* _help, const char* rname)
    : opts(_o), argc(_argc), argv(_argv), help(_help), regname(rname)
{
    int sidx = 1;

    for (idx = 1; idx < *argc; idx++)
    {
	if (argv[idx][0] == '-')
	{
	    int olong = (argv[idx][1] == '-');
	    if (olong && argv[idx][2] == 0)
		break; // end of options
	    if (findOpt(olong) == 0)
		continue;
	}
	else if (sidx != idx)
	{
	    //printf("SIDX\n");
	    argv[sidx] = argv[idx];
	}
	sidx++;
    }

    while (idx < *argc && sidx != idx)
    {
	//printf("SIDX2\n");
	argv[sidx++] = argv[idx++];
    }

    *argc = sidx;
}

Args::~Args()
{
}

int Args::findOpt(int olong)
{
    int i = 0;
    char* arg = argv[idx] + olong + 1;
    olong++;

    char* par = strchr(arg, '=');
    if (par)
    {
        *par = 0;
	par++;
    }
    else if ((idx + 1) < *argc)
    {
	par = argv[++idx];
    }

    avm::vector<const Option*> ol;
    ol.push_back(opts);
    const Option* o = 0;
    while (ol.size())
    {
	o = ol.front();
        ol.pop_front();
	for (; o->type != Option::NONE; o++)
	{
	    //printf("OPTION %d   '%s'  %d   '%s' '%s'\n", o[i].type, arg, olong, o[i].oshort, o[i].olong);
	    if (o->type == Option::HELP
		&& (strcasecmp(arg, "h") == 0
		    || strcmp(arg, "?") == 0
		    || strcasecmp(arg, "help") == 0))
		break;
	    if (o->type == Option::OPTIONS)
	    {
		ol.push_back((const Option*) o->value);
                continue;
	    }

	    if (olong < 2)
	    {
		if (o->oshort && strcmp(arg, o->oshort) == 0)
		    break;
	    }
	    else if (o->olong && strcmp(arg, o->olong) == 0)
		break;
	}
	if (o->type != Option::NONE)
	    break;
    }

    switch (o->type)
    {
    default:
    case Option::NONE:
	return -1;
    case Option::BOOL:
    case Option::REGBOOL:
        if (!read_bool(o, arg, par, regname))
	    idx--; // no argument given
	break;
    case Option::DOUBLE:
    case Option::REGDOUBLE:
        read_double(o, arg, par, regname);
	break;
    case Option::INT:
    case Option::REGINT:
	read_int(o, arg, par, regname);
	break;
    case Option::STRING:
    case Option::REGSTRING:
	read_string(o, arg, par, regname);
	break;
    case Option::CODEC:
	printf("PARSER %s\n", par);
	parse_codec(o, par);
        break;
    case Option::SUBOPTIONS:
	parse_suboptions((const Option*)o->value, arg, par, regname);
        break;
    case Option::HELP:
	printf("\nUsage: %s %s\n\n", argv[0], help);
	show_help(opts, true);
	exit(0);
    }
    return 0;
}

AVM_END_NAMESPACE;
