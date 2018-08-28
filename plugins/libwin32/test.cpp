
#include "avm_default.h"
#include "avifile.h"
#include "except.h"
#include "utils.h"

#ifdef USE_MPATROL
#include "mpatrol.h"
#endif
#include <stdio.h>

//#include <vector>

// test for compatibility with std::vector
// this test probably should be here - but it's not
// that important I guess...

int main(int argc, char* argv[])
{
    avm::vector<int> a1;
    avm::vector<int> b1;

    a1.push_back(1);
    a1.push_back(2);
    a1.push_back(3);

    b1.push_back(4);
    b1.push_back(5);
    b1.push_back(6);

    avm::vector<int>& c1 = a1;

    for (unsigned i = 0; i < a1.size(); i++)
	printf("elem:%d  %d\n", i, a1[i]);

    a1 = b1;
    b1[0] = 0;
    c1[1] = 2;
    for (unsigned i = 0; i < a1.size(); i++)
	printf("elem:%d  %d\n", i, a1[i]);

    return 0;
}
