#include "AsxReader.h"
#include "avm_output.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

AVM_BEGIN_NAMESPACE;

enum asx_position
{
    begin,
    open_bracket,
    close_bracket,
    in_comment,
    in_quoted_comment,
    in_tagname,
    after_tagname,
    in_endtag,
    before_tagclose,
    in_propname,
    after_propname,
    prop_equal,
    after_propequal,
    between_props,
    in_propcontents,
    in_quoted_propcontents,
    in_quoted_contents,
    in_contents,
};

static inline bool is_whitespace(char c)
{
    return ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'));
}

bool ASX_Reader::addURL(const char* url)
{
    int i = 0;

    while (*url && (!isprint(*url) || (*url == '"')))
	url++;

    while (isprint(url[i]) && (url[i] != '"'))
	i++;

    if (i > 0)
    {
	avm::string vurl = avm::string(url, i);

	if (strncasecmp(vurl.c_str(), "mms://", 6) != 0
	    && strncasecmp(vurl.c_str(), "http://", 7) != 0)
	{
	    char* h = new char[m_Server.size() * 2 + m_Filename.size()];
	    int p = sprintf(h, "http://%s", m_Server.c_str());
	    if (vurl[0] != '/')
	    {
		h[p++] = '/';
		strcpy(h + p, m_Filename.c_str());
		char* q = strchr(h + p, '?'); // strip all after '?'
                if (q) *q = 0;
	    }
	    //printf("_____addUrl>%s<>%s<____\n", h, vurl.c_str());
	    vurl.insert(0, h);
	}

	//printf("URL:%s<\n", vurl.c_str());
	m_Urls.push_back(vurl);
        return true;
    }
    return false;
}

bool ASX_Reader::create(const char* data, uint_t size)
{
    int last_start = 0;
    avm::string last_tag;
    avm::string last_prop;
    avm::string last_prop_val;
    m_Urls.clear();

    //printf("PARSE >%s< %d\n", data, size);
    asx_position state = begin;
    for (unsigned i = 0; i < size; i++)
    {
	unsigned char c = data[i];
	if (c < 6) break; // stop processing
	//printf("i %d %c   %d\n", i, c, state);
	int is_alpha = isalpha(c) || (c >= 0x80);
	int is_space = is_whitespace(c);
	switch(state)
	{
	case begin:
	    if (c == '<')
		state = open_bracket;
	    else if (is_alpha)
	    {
                // sometime only single one line redirection appears
		if (!strncasecmp(data + i, "mms://", 6)
                    || !strncasecmp(data + i, "http://", 7))
                    return addURL(data + i);
	    }
            continue;
	case open_bracket:
	    if (c == '!')
		state = in_comment;
	    else if (c == '/')
	    {
		state = in_endtag;
		last_start = i + 1;
	    }
	    else if (is_alpha)
	    {
		state = in_tagname;
		last_start = i;
	    }
            else if (!is_space)
		return false;
	    continue;
	case in_comment:
	    if (c == '"')
		state = in_quoted_comment;
	    else if (c == '>')
		state = close_bracket;
	    continue;
	case in_quoted_comment:
	    if (c == '"')
		state = in_comment;
	    continue;
	case close_bracket:
	    if (c == '<')
		state = open_bracket;
            else
		state = in_contents;
	    continue;
	case in_tagname:
	    if (is_space)
	    {
		last_tag = avm::string(&data[last_start], i-last_start);
		//printf("TAG1: %s\n", last_tag.c_str());
		state = after_tagname;
	    }
	    else if ((c == '/') || (c == '>'))
	    {
		last_tag = avm::string(&data[last_start], i-last_start);
		//printf("TAG2: %s\n", last_tag.c_str());
		if (c == '/')
	    	    state = before_tagclose;
		else
		    state = close_bracket;
	    }
	    continue;
	case after_tagname:
	    if (c == '/')
		state = before_tagclose;
	    else if (!is_space)
	    {
		last_start = i;
		state = in_propname;
	    }
	    continue;
	case in_endtag:
	    if (c == '>')
	    {
		last_tag = avm::string(&data[last_start], i-last_start);
		state = close_bracket;
		AVM_WRITE("ASX reader", 1, " TAG: %s\n", last_tag.c_str());
		if (strcasecmp(last_tag.c_str(), "asx")==0)
		    return true;
	    }
	    continue;
	case before_tagclose:
	    if (c == '>')
		state = close_bracket;
	    else if (!is_space)
		return false;
	    continue;
	case in_propname:
	    if (c == '=')
	    {
		last_prop = avm::string(&data[last_start], i-last_start);
		state = prop_equal;
	    }
	    else if (is_space)
	    {
		last_prop = avm::string(&data[last_start], i-last_start);
		state = after_propname;
	    }
	    continue;
	case after_propname:
	    if (c == '=')
		state = prop_equal;
	    continue;
	case prop_equal:
	    if (is_space)
		continue;
	    if (c == '"')
		state = in_quoted_propcontents;
	    else
	        state = in_propcontents;
	    last_start = i;
	    continue;
	case in_propcontents:
	    if (c == '"')
		state = in_quoted_propcontents;
	    else if (is_space || (c == '/') || (c == '>'))
	    {
		last_prop_val = avm::string(&data[last_start], i-last_start);
		AVM_WRITE("ASX reader", 1, "VALUE: %s  t:%s p:%s\n", last_prop_val.c_str(), last_tag.c_str(), last_prop.c_str());
		if (strcasecmp(last_tag.c_str(), "ref") == 0
		    || strcasecmp(last_tag.c_str(), "entryref") == 0
		    || strcasecmp(last_tag.c_str(), "a") == 0
		    || strcasecmp(last_tag.c_str(), "location") == 0
                    || strcasecmp(last_tag.c_str(), "embed") == 0) // part of some javascript
		    if (strcasecmp(last_prop.c_str(), "href") == 0
			|| strcasecmp(last_prop.c_str(), "src") == 0)
			addURL(last_prop_val.c_str());
		if (c == '/')
		    state = before_tagclose;
		else if (c == '>')
		    state = close_bracket;
		else
		    state = between_props;
	    }
	    continue;
	case in_quoted_propcontents:
	    if (c == '"')
		state = in_propcontents;
	    continue;
	case between_props:
	    if (c == '/')
		state = before_tagclose;
	    else if (c == '>')
	        state = close_bracket;
	    else if (is_alpha)
	    {
		last_start = i;
		state = in_propname;
	    }
	    else if (!is_space)
		return false;
	    continue;
	case in_contents:
	    if (c == '"')
		state = in_quoted_contents;
	    else if (c == '<')
		state = open_bracket;
	    continue;
	case in_quoted_contents:
	    if (c == '"')
		state = in_contents;
	    continue;
	default: // after_propequal
	    AVM_WRITE("ASX reader", "FIXME ERROR after_propequal not handled\n");
            break;
	}
    }
    return (m_Urls.size() > 0);
}

bool ASX_Reader::getURLs(avm::vector<avm::string>& urls) const
{
    //for (unsigned i = 0; i < m_Urls.size(); i++) printf("URLs: %s\n", m_Urls[i].c_str());
    urls = m_Urls;
    return true;
}

AVM_END_NAMESPACE;
