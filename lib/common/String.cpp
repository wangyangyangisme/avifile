#include "avm_stl.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

AVM_BEGIN_NAMESPACE;

static char empty_string[] = "";
string::string()
{
    //str = new char[1];
    //str[0] = 0;
    str = empty_string;
}

string::string(char s)
{
    str = new char[2];
    str[0] = s;
    str[1] = 0;
}

string::string(const char* s, uint_t len)
{
    uint_t slen = (s) ? strlen(s) : 0;
    if (len == 0 || len > slen)
	len = slen;
    str = new char[len + 1];
    if (s)
	memcpy(str, s, len);
    str[len] = 0;
}

string::string(const string& s, uint_t len)
{
    if (len == 0)
	len = s.size();

    str = new char[len + 1];
    memcpy(str, s.str, len);
    str[len] = 0;
}

string::~string()
{
    if (str != empty_string)
    {
	//AVM_WRITE("Init", 0, "delete %s\n", str);
	delete[] str;
    }
}

bool string::operator==(const char* s) const
{
    if (s)
	return !strcmp(str, s);
    return (size() == 0);
}

bool string::operator<(const string& s) const
{
    return (strcmp(str, s.str)<0);
}

string& string::operator=(const char* s)
{
    if (str != s)
    {
	uint_t sz = (!s) ? 0 : strlen(s);
	if (str != empty_string)
	    delete[] str;
	if (sz > 0)
	{
	    str = new char[sz + 1];
	    memcpy(str, s, sz);
	    str[sz] = 0;
	}
	else
	    str = empty_string;
    }
    return *this;
}

string& string::operator+=(const char* s)
{
    if (s)
    {
	uint_t s1 = size();
	uint_t s2 = strlen(s);
	if (s2 > 0)
	{
	    s2 += s1;
	    char* p = new char[s2 + 1];
	    memcpy(p, str, s1);
	    memcpy(p + s1, s, s2 - s1);
	    p[s2] = 0;
	    if (str != empty_string)
		delete[] str;
	    str = p;
	}
    }
    return *this;
}

string& string::erase(uint_t from, uint_t to)
{
    char* p = str + from;
    if (to != npos && to > 0 && to < size())
    {
        // add check for size() ???
	char* i = p + to;
	while (*i)
	    *p++ = *i++;
    }
    if (p == str)
    {
	if (str != empty_string)
	    delete[] str;
        str = empty_string;
    }
    else
	*p = 0;
    return *this;
}

void string::insert(uint_t pos, const string& s)
{
    uint_t l = s.size();
    uint_t k = size();
    char* p = new char[k + l + 1];
    strcpy(p, str);
    strcpy(p + pos, s.str);
    strcpy(p + pos + l, str + pos);
    if (str != empty_string)
	delete[] str;
    str = p;
    str[k + l] = 0;
}

string::size_type string::find(const string& s, size_type startpos) const
{
    char* p = strstr(str + startpos, s.str);
    return (p) ? p - str : npos;
}

string::size_type string::find(char c) const
{
    const char* p = strchr(str, c); return p ? (p-str) : npos;
}

string::size_type string::rfind(char c) const
{
    const char* p = strrchr(str, c); return p ? (p-str) : npos;
}

int string::sprintf(const char* fmt, ...)
{
    int r;
    char* s = 0;
    va_list ap;
    va_start(ap, fmt);
    if (str != empty_string)
    {
	delete[] str;
        str = empty_string;
    }
#ifdef _GNU_SOURCE
    r = vasprintf(&str, fmt, ap);
#else
    // a bit poor hack but should be sufficient
    // eventually write full implementation
    s = malloc(1000);
    r = vsnprintf(str, 999, fmt, ap);
#endif
    if (s)
    {
	str = new char[r + 1];
	memcpy(str, s, r);
	str[r] = 0;
    }
    else
    {
	str = empty_string;
        r = 0;
    }

    return r;
}

AVM_END_NAMESPACE
