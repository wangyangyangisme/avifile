#include "avm_stl.h"

class ISubRenderer
{
public:
    virtual ~ISubRenderer() {}
    virtual int Open(const char* fontname) =0;
    virtual int Draw(const char* lines) =0;

protected:
    virtual int addChar(int i); // add one char to current line
    virtual int getWidth(); // get actual width of current text
    virtual int parse();

    avm::vector<avm::string> m_Lines;
};
