#include "Statistic.h"
#include <string.h>

AVM_BEGIN_NAMESPACE;

Statistic::Statistic(const char* name, uint_t size)
    :m_pName(name), m_pValues(0), m_uiSize(size), m_uiPos(0), m_dSum(0)
{
}

Statistic::Statistic(const Statistic& s)
{
    m_pName = s.m_pName;
    m_uiSize = s.m_uiSize;
    m_pValues = new float[m_uiSize];
    memcpy(m_pValues, s.m_pValues, sizeof(float) * m_uiSize);
    m_dSum = s.m_dSum;
    m_uiPos = s.m_uiPos;
}

void Statistic::clear()
{
    m_dSum = 0;
    delete[] m_pValues;
    m_pValues = 0;
}

void Statistic::insert(float v)
{
    if (!m_pValues)
    {
	m_pValues = new float[m_uiSize];
	for (unsigned i = 0; i < m_uiSize; i++)
	    m_pValues[i] = v;
        m_dSum = v * (m_uiSize);
    }

    m_dSum -= m_pValues[m_uiPos];
    m_pValues[m_uiPos] = v;
    m_dSum += v;

    m_uiPos = (m_uiPos + 1) % m_uiSize;
}

AVM_END_NAMESPACE;
