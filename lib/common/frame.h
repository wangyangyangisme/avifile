#ifndef AVIFILE_FRAME_H
#define AVIFILE_FRAME_H

#include "image.h"

// internal representation for one frame
AVM_BEGIN_NAMESPACE;

class frame
{
    CImage *m_pData;
    double m_dTime;
    framepos_t m_uiPos;
public:
    frame() : m_pData(0), m_dTime(-1.0), m_uiPos(0) {}
    ~frame()
    {
	if (m_pData)
	    m_pData->Release();
    }
    CImage* getData() { return m_pData; }
    framepos_t getPos() { return m_uiPos; }
    double getTime() { return m_dTime; }
    void setData(CImage* newData)
    {
	if (m_pData)
	    m_pData->Release();
	m_pData = newData;
    }
    void setDataQuality(float q)
    {
	if (m_pData)
            m_pData->SetQuality(q);
    }
    float getDataQuality()
    {
	return (m_pData) ? m_pData->GetQuality() : 0.0f;
    }
    void setPos(framepos_t p) {	m_uiPos = p; }
    void setTime(double t) { m_dTime = t; }
};

AVM_END_NAMESPACE;

#endif // AVIFILE_FRAME_H
