#ifndef AVIFILE_STATISTIC_H
#define AVIFILE_STATISTIC_H

#include "avm_stl.h"

AVM_BEGIN_NAMESPACE;

class Statistic
{
    static const int STAT_SIZE = 25;
    avm::string m_pName;
    float* m_pValues;
    uint_t m_uiSize;
    uint_t m_uiPos;
    double m_dSum;

public:
    explicit Statistic(const char* name, uint_t msize = STAT_SIZE);
    Statistic(const Statistic& s);
    ~Statistic() { delete[] m_pValues; }

    float average() const { return m_dSum / m_uiSize; }
    void clear();
    void insert(float v);
    const char* getName() const { return (const char*) m_pName; }
};

AVM_END_NAMESPACE;

#endif // AVIFILE_STATISTIC_H
