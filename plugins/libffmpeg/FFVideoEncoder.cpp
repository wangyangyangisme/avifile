#include "FFVideoEncoder.h"
#include "fillplugins.h"
#include <stdio.h>
#include <string.h>

AVM_BEGIN_NAMESPACE;

static void libffmpeg_set_attr(avm::vector<AttributeInfo>& a, AVCodec* codec)
{
    if (!codec)
	return;

    const AVOption* c = 0;//codec->options;
#if 0
    if (c) {
	const AVOption *stack[FF_OPT_MAX_DEPTH];
	int depth = 0;
	for (;;) {
            //printf("NAME  %s\n", c->name);
	    if (!c->name) {
		if (c->help) {
		    stack[depth++] = c;
		    c = (const AVOption*)c->help;
		} else {
		    if (depth == 0)
			break; // finished
		    c = stack[--depth];
                    c++;
		}
	    } else {
		int t = c->type & FF_OPT_TYPE_MASK;
		char* arr[100];
                char* dealloc;
#if 0
		printf("Config   %s  %s\n",
		       t == FF_OPT_TYPE_BOOL ? "bool   " :
		       t == FF_OPT_TYPE_DOUBLE ? "double  " :
		       t == FF_OPT_TYPE_INT ? "integer" :
		       t == FF_OPT_TYPE_STRING ? "string " :
		       "unknown??", c->name);
#endif
		switch (t) {
		case FF_OPT_TYPE_BOOL:
		    a.push_back(AttributeInfo(c->name, c->help,
					      AttributeInfo::Integer,
					      0, 1, (int)c->defval));
		    break;
		case FF_OPT_TYPE_DOUBLE:
		    a.push_back(AttributeInfo(c->name, c->help,
                                              c->defval, c->min,c->max));
		    break;
		case FF_OPT_TYPE_INT:
		    a.push_back(AttributeInfo(c->name, c->help,
					      AttributeInfo::Integer,
					      (int)c->min, (int)c->max,
					      (int)c->defval));
		    break;
		case FF_OPT_TYPE_STRING:
		    t = 0;
		    dealloc = 0;
		    if (c->defstr)
		    {
			char* tmp = dealloc = strdup(c->defstr);
			if (tmp)
			{
                            char* semi;
			    while (tmp && (semi = strchr(tmp, ';')))
			    {
				*semi++ = 0;
				arr[t++] = tmp;
				if (t > 97)
				    break; // weird - too many string options ???
				tmp = semi;
			    }
			    arr[t++] = tmp;
			}
			arr[t] = 0;
		    }
		    //printf("%d   %s\n", i, cnf->name);
		    a.push_back((t > 1) ? AttributeInfo(c->name, c->help, (const char**)arr, (int)c->defval)
				: AttributeInfo(c->name, c->help, AttributeInfo::String));
		    if (dealloc)
			free(dealloc);
		    break;
/*
		    if (c->defstr) {
			char* d = av_strdup(c->defstr);
			char* f = strchr(d, ',');
			if (f)
                            *f = 0;
			i += sprintf(def + i, "%s%s=%s",
				     col, c->name, d);
                        av_free(d);
		    }
		    break;
                    */
		}
		c++;
	    }
	}
    }
#endif
}


FFVideoEncoder::FFVideoEncoder(AVCodec* av, const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
    :IVideoEncoder(info), m_pAvCodec(av), m_pAvContext(0), m_Caps(CAP_YV12),
    m_bh(bh), m_obh(bh)
{
    m_obh.biCompression = info.fourcc;
    m_obh.biHeight = labs(m_obh.biHeight);
}

FFVideoEncoder::~FFVideoEncoder()
{
    Stop();
}

int FFVideoEncoder::EncodeFrame(const CImage* src, void* dest, int* is_keyframe, uint_t* size, int* lpckid)
{
    int hr = 0;

    if (!m_pAvContext)
    {
	m_pAvContext = avcodec_alloc_context();
        m_pAvContext->width = m_bh.biWidth;
	m_pAvContext->height = m_obh.biHeight;
	//m_pAvContext->pix_fmt = PIX_FMT_YUV420P;

	m_pAvContext->bit_rate = 1000000;
#warning fixme
//	m_pAvContext->frame_rate_base = 1000000;
//	m_pAvContext->frame_rate = (int)(m_fFps * m_pAvContext->frame_rate_base + 0.5);
	m_pAvContext->gop_size = 250;
	m_pAvContext->qmin = 2;
	m_pAvContext->qmax = 31;
	//m_pAvContext->flags = CODEC_FLAG_HQ;

        printf("CODEC opening  %dx%d\n", m_bh.biWidth, m_obh.biHeight);

	if (avcodec_open(m_pAvContext, m_pAvCodec) < 0)
	{
	    free(m_pAvContext);
	    m_pAvContext = 0;
	    return -1;
	}
#if 0
	if (!codec)
	    return;
	const AVOption* c = codec->options;
	if (c) {
	    const AVOption *stack[FF_OPT_MAX_DEPTH];
	    int depth = 0;

	    for (;;) {
	    double d;

	    //printf("NAME  %s\n", c->name);
	    if (!c->name) {
		if (c->help) {
		    stack[depth++] = c;
		    c = (const AVOption*)c->help;
		} else {
		    if (depth == 0)
			break; // finished
		    c = stack[--depth];
                    c++;
		}
	    } else {
		int t = c->type & FF_OPT_TYPE_MASK;
		char* arg[200];
                char* sval = 0;

		switch (t) {
		case FF_OPT_TYPE_BOOL:
		    PluginGetAttrInt(m_Info, c->name, &ival);
		    sprintf(arg, "%150s=%d", c->name, ival);
		    break;
		case FF_OPT_TYPE_DOUBLE:
		    PluginGetAttrFloat(m_Info, c->name, &dval);
		    sprintf(arg, "%150s=%f", c->name, dval);
		    break;
		case FF_OPT_TYPE_INT:
		    PluginGetAttrInt(m_Info, c->name, &ival);
		    sprintf(arg, "%150s=%d", c->name, ival);
		    break;
		case FF_OPT_TYPE_STRING:
		    PluginGetAttrString(m_Info, c->name, &sval);
		    if (sval)
			sprintf(arg, "%s=%s", c->name, d);
		    break;
		}
		col = ":";
		c++;


		avoption_parse(m_pAvContext, codec->options, arg);
	    }
#endif
    }

    CImage* ci = 0;
    //printf("CODEC format  0x%x\n", src->Format());
    if (src->Format() != IMG_FMT_YV12)
    {
        printf("Converted\n");
        ci = new CImage(src, IMG_FMT_YV12);
    }
    else
        ci = (CImage*) src;

    AVFrame f; // YV12 only
    memset(&f, 0, sizeof(AVFrame));
    f.data[0] = ci->Data(0);
    f.data[1] = ci->Data(2);
    f.data[2] = ci->Data(1);
    f.linesize[0] = src->Stride(0);
    f.linesize[1] = src->Stride(2);
    f.linesize[2] = src->Stride(1);
    //printf("ECDING FF  %p %p %p   sz:%d\n", f.data[0], f.data[1], f.data[2], GetOutputSize());
    //printf("ECDING FF  %d %d %d\n", f.linesize[0], f.linesize[1], f.linesize[2]);

    int rsize = avcodec_encode_video(m_pAvContext, (unsigned char*) dest,
				     GetOutputSize(), &f);
    //printf("ECDING FF  size %d\n", rsize);
    if (size)
	*size = rsize;
    if (is_keyframe) {
	*is_keyframe = m_pAvContext->coded_frame->key_frame ? 16 : 0;
	//printf("KEYFRAME %d\n", *is_keyframe);
    }
    if (ci != src)
        ci->Release();

    return hr;
}

int FFVideoEncoder::Start()
{
    return 0;
}

int FFVideoEncoder::Stop()
{
    if (m_pAvContext)
    {
	avcodec_close(m_pAvContext);
	free(m_pAvContext);
        m_pAvContext = 0;
    }

    return 0;
}

int FFVideoEncoder::SetQuality(int quality)
{
    if (quality < 0 || quality >10000)
	return - 1;
    m_iQual = quality;

    return 0;
}

int FFVideoEncoder::SetKeyFrame(int freq)
{
    return 0;
}

int FFVideoEncoder::SetFps(float fps)
{
    m_fFps = fps;
    return 0;
}

AVM_END_NAMESPACE;
