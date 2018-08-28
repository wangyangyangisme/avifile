/********************************************************
 *
 *       Video renderers
 *       Copyright 2000 Eugene Kuznetsov (divx@euro.ru)
 *
 ********************************************************/


#include "rendlock.h"
#include "subrend.h"

#include "avm_fourcc.h"
#include "avm_except.h"
#include "mmx.h"
#include "avm_output.h"
#define DECLARE_REGISTRY_SHORTCUT
#include "configfile.h"
#undef DECLARE_REGISTRY_SHORTCUT

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <langinfo.h>

bool avm::IVideoRenderer::allow_sw_yuv=true;//false;

#ifndef X_DISPLAY_MISSING

#include <errno.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput.h> //XGetExtensionVersion
#include <X11/extensions/XShm.h>

#ifdef HAVE_LIBXXF86DGA
#include <X11/extensions/xf86dga.h>
#endif

#ifdef HAVE_LIBXXF86VM
#include <X11/extensions/xf86vmode.h>
#endif

//#undef HAVE_LIBXFT
#ifdef HAVE_LIBXFT
#include <X11/Xft/Xft.h>
//#include <X11/extensions/Xrender.h>
#endif

#ifdef HAVE_LIBXV
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#endif

#include "VideoDPMS.h"

#ifdef HAVE_LIBSDL
#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include <SDL_mouse.h>
#endif // HAVE_LIBSDL

#if HAVE_VIDIX
#include <vidix.h>
#include <vidixlib.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define __MODULE__ "VideoRenderer"

AVM_BEGIN_NAMESPACE;

int GetPhysicalDepth(void* _dpy)
{
    Display* dpy = (Display*) _dpy;
    if (!dpy)
	return 0;

    int screen = DefaultScreen(dpy);
    int planes = DefaultDepth(dpy, screen);
    int n, pixmap_bits = 0;
    XPixmapFormatValues *pf = XListPixmapFormats(dpy, &n);

    for (int i = 0; i < n; i++)
    {
	//cout << i << " depth: " << pf[i].depth << "  bpp: " << pf[i].bits_per_pixel << endl;
	if (pf[i].depth == planes)
	{
	    pixmap_bits = pf[i].bits_per_pixel;
            break;
	}
    }
    XFree(pf);

    if (pixmap_bits == 16 && DefaultVisual(dpy, screen)->red_mask == 0x7c00)
	pixmap_bits = 15;

    return pixmap_bits;
}


#ifdef HAVE_LIBSDL

#ifndef SDL_VERSIONNUM
#define SDL_VERSIONNUM(X, Y, Z)  (X)*1000 + (Y)*100 + (Z)
#endif

#define _SDL_VER SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL)

#ifndef SDL_DISABLE
#define SDL_DISABLE 0
#endif

#ifndef SDL_ENABLE
#define SDL_ENABLE 1
#endif

#ifndef SDL_BUTTON_LEFT
#define SDL_BUTTON_LEFT		1
#define SDL_BUTTON_MIDDLE	2
#define SDL_BUTTON_RIGHT	3
#endif

#ifndef SDL_BUTTON_WHEELUP
#define SDL_BUTTON_WHEELUP      4
#define SDL_BUTTON_WHEELDOWN    5
#endif

/* XPM */
static const char* mouse_arrow[] =
{
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "X                               ",
  "XX                              ",
  "X.X                             ",
  "X..X                            ",
  "X...X                           ",
  "X....X                          ",
  "X.....X                         ",
  "X......X                        ",
  "X.......X                       ",
  "X........X                      ",
  "X.....XXXXX                     ",
  "X..X..X                         ",
  "X.X X..X                        ",
  "XX  X..X                        ",
  "X    X..X                       ",
  "     X..X                       ",
  "      X..X                      ",
  "      X..X                      ",
  "       XX                       ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "0,0"
};

static const char* mouse_zoomlu[] =
{
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
  "X..............................X",
  "X..............................X",
  "X..XXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "X..X                            ",
  "XXXX                            ",
  "0,0"
};

static const char* mouse_zoomrb[] =
{
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                            XXXX",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "                            X..X",
  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX..X",
  "X..............................X",
  "X..............................X",
  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
  "31,31"
};

static const char* hidden_arrow[] =
{
  /* width height num_colors chars_per_pixel */
  "    8    1        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "        ",
  "0,0"
};

static SDL_Cursor* init_system_cursor(const char* image[])
{
    int i, row, col;
    int hot_x, hot_y;
    Uint8* data, *mask;
    int rows, cols, colors, per_pix;
    int s;
    SDL_Cursor* cret;
    sscanf(image[0], "%d %d %d %d", &cols, &rows, &colors, &per_pix);

    //printf("col %d  row %d  colo: %d  %d\n", cols, rows, colors, per_pix);
    s = ((cols + 7) / 8) * rows;
    data = new Uint8[s];
    mask = new Uint8[s];
    memset(data, 0, s);
    memset(mask, 0, s);
    colors++; // skip definition

    i = -1;
    for ( row=0; row<rows; ++row ) {
	for ( col=0; col<cols; ++col ) {
	    if ( col % 8 ) {
		data[i] <<= 1;
		mask[i] <<= 1;
	    } else {
		++i;
		data[i] = mask[i] = 0;
	    }
	    switch (image[colors+row][col]) {
	    case 'X':
		data[i] |= 0x01;
		mask[i] |= 0x01;
		break;
	    case '.':
		mask[i] |= 0x01;
		break;
	    case ' ':
		break;
	    }
	}
    }
    sscanf(image[colors + row], "%d,%d", &hot_x, &hot_y);
    cret = SDL_CreateCursor(data, mask, cols, rows, hot_x, hot_y);
    delete[] data;
    delete[] mask;
    return cret;
}

class MouseKeeper
{
    int64_t m_lMouseTime;
    int64_t m_lMouseLast; // keep time of last mouse event
    int m_iMouseX, m_iMouseY;
    int m_iMouseOff;
    int m_iX, m_iY;
    SDL_Cursor* m_pCursor;// store actuall mouse cursor
    SDL_Cursor* m_pCursorHide;
    SDL_Cursor* m_pCursorZoomLU;
    SDL_Cursor* m_pCursorZoomRB;
public:
    MouseKeeper() :  m_lMouseTime(0), m_lMouseLast(0),
	m_iMouseX(0), m_iMouseY(0), m_iMouseOff(0),
	m_pCursor(0), m_pCursorHide(0),
	m_pCursorZoomLU(0), m_pCursorZoomRB(0)
    {
    }
    ~MouseKeeper()
    {
    }
    void Open()
    {
	m_pCursorHide = init_system_cursor(hidden_arrow);
	m_pCursorZoomLU = init_system_cursor(mouse_zoomlu);
	m_pCursorZoomRB = init_system_cursor(mouse_zoomrb);
    }
    void Close()
    {
	if (m_pCursorHide)
	    SDL_FreeCursor(m_pCursorHide);
	if (m_pCursorZoomLU)
	    SDL_FreeCursor(m_pCursorZoomLU);
	if (m_pCursorZoomRB)
	    SDL_FreeCursor(m_pCursorZoomRB);
    }
    void On() {
	if (m_pCursor)
	    SDL_SetCursor(m_pCursor);
    }
    void Off() {
	m_lMouseTime = 0;
	m_iMouseOff = 3; // resistance
	SDL_Cursor* tmp = SDL_GetCursor();
	if (tmp != m_pCursorHide)
	{
	    m_pCursor = tmp;
	    SDL_SetCursor(m_pCursorHide);
	}
    }
    void Process() {
	if (m_lMouseTime)
	{
	    if (to_float(longcount(), m_lMouseTime) > 1.0)
		Off();
	    else if (m_lMouseTime != m_lMouseLast && --m_iMouseOff == 0)
		On();
	    m_lMouseLast = m_lMouseTime;
	}
    }
    void Set(int x, int y)
    {
	m_iMouseX = x;
	m_iMouseY = y;
        m_lMouseTime = longcount();
    }
    void Store()
    {
     	// save original mouse position
        // using mouse motion event
	m_iX = m_iMouseX;
        m_iY = m_iMouseY;
    }
    void Restore()
    {
	// recover original mouse position
        // called with LOCK
        SDL_WarpMouse(m_iX, m_iY); // this seems to be doing same thing
    }
};

#endif // HAVE_LIBSDL

#ifdef HAVE_ICONV
#include <iconv.h>

XFontSet XLoadQueryFontSet(Display *disp,const char *fontset_name)
{
    XFontSet fontset;
    int missing_charset_count;
    char **missing_charset_list;
    char *def_string;

    fontset = XCreateFontSet(disp, fontset_name,
			     &missing_charset_list, &missing_charset_count,
			     &def_string);
    if (missing_charset_count) {
	AVM_WRITE("renderer", "Missing charsets in FontSet(%s) creation.\n",
		  fontset_name);
	XFreeStringList(missing_charset_list);
    }
    return fontset;
}

#define XLoadQueryFont XLoadQueryFontSet
#define XFreeFont XFreeFontSet
#define XDrawString(d,w,gc,x,y,s,l)  XmbDrawString(d,w,font,gc,x,y,s,l)
#define XDrawImageString(d,w,gc,x,y,s,l)  XmbDrawImageString(d,w,font,gc,x,y,s,l)
#define XTextWidth XmbTextEscapement
#define XftTextExtents8 XftTextExtentsUtf8
#define XftDrawString8 XftDrawStringUtf8
#endif /* HAVE_ICONV */

void* VideoRendererWithLock::eventThread(void *arg)
{
    VideoRendererWithLock* vr = (VideoRendererWithLock*) arg;
    avm_usleep(100000);
    while (!vr->m_bQuit)
	vr->processEvent();
    return 0;
}

#if _SDL_VER > 1104
class SDLGRtConfig : public IRtConfig
{
    VideoRendererWithLock* vr;
    avm::vector<AttributeInfo> vattrs;
    avm::vector<int> val;
public:
    SDLGRtConfig(VideoRendererWithLock* _vr)
	: vr(_vr)
    {
	vattrs.push_back(AttributeInfo("SDL_GAMMA_RED", "Gamma Red",
				       AttributeInfo::Integer,
				       0, 30, 10));
	vattrs.push_back(AttributeInfo("SDL_GAMMA_GREEN", "Gamma Green",
				       AttributeInfo::Integer,
				       0, 30, 10));
	vattrs.push_back(AttributeInfo("SDL_GAMMA_BLUE", "Gamma Blue",
				       AttributeInfo::Integer,
				       0, 30, 10));
        val.resize(vattrs.size());
	for (unsigned i = 0; i < vattrs.size(); i++)
	    val[i] = RegReadInt("aviplay", vattrs[i].GetName(), 10);

	setGamma();
    }
    virtual ~SDLGRtConfig()
    {
	// restore to defaults or at least those from the begining
	for (unsigned i = 0; i < vattrs.size(); i++)
	    RegWriteInt("aviplay", vattrs[i].GetName(), val[i]);

	val[0] = val[1] = val[2] = 10;
	setGamma();
    }
    // IRtConfig interface
    virtual const avm::vector<AttributeInfo>& GetAttrs() const
    {
	return vattrs;
    }
    virtual int GetValue(const char* attr, int* result) const
    {
	AVM_WRITE("renderer", 1, "sdlg: get %s\n", attr);
	for (unsigned i = 0; i < vattrs.size(); i++)
	    if (attr == vattrs[i].GetName())
	    {
		*result = val[i];
		return 0;
	    }

        return -1;
    }
    virtual int SetValue(const char* attr, int value)
    {
	AVM_WRITE("renderer", 1, "sdlg: set %s value: %d\n", attr, value);
	for (unsigned i = 0; i < vattrs.size(); i++)
	    if (attr == vattrs[i].GetName())
	    {
		val[i] = value;
                setGamma();
		return 0;
	    }

        return -1;
    }
private:
    void setGamma()
    {
	vr->Lock();
	SDL_SetGamma(val[0]/10.0, val[1]/10., val[2]/10.);
	vr->Unlock();
    }
};
#else
class SDLGRtConfig : public IRtConfig
{
public:
    virtual int GetValue(const char* attr, int* result) const { return 0; }
    virtual int SetValue(const char* attr, int value) { return 0; }
};

#endif // _SDL_VER > 1104

#undef __MODULE__
#define __MODULE__ "Fullscreen renderer"

#ifdef HAVE_LIBSDL

// according to SDL we should not use this
// new XFree should have this fixed
//static EnvKeeper keeper;
static const char* sdl_var = "SDL_VIDEO_X11_NODIRECTCOLOR";
class EnvKeeper
{
    char* str;
public:
    EnvKeeper()
    {
	str = getenv(sdl_var);
	if (str)
	{
	    char* tmp = new char[strlen(str)+1];
	    strcpy(tmp, str);
	    str = tmp;
	}
	avm_setenv(sdl_var, "1", true);
    }
    ~EnvKeeper()
    {
	if (!str)
	    avm_unsetenv(sdl_var);
	delete str;
    }
};

class FullscreenRenderer: public VideoRendererWithLock
{
protected:
    Display* dpy;
    GC xgc;
    ISubRenderer* m_pSubRenderer;
    SDL_Event keyrepev;
    SDL_Rect cliprect;
    SDL_SysWMinfo info;
    SDL_Surface* screen;
    SDLGRtConfig* m_pSdlgRtConfig;
    MouseKeeper m_Mouse;
    int max_w, max_h;
    SDL_Rect m_Zoom;
    int fs;
    int bit_depth;
    int bpp;
    const CImage* image;
    char *convbuf;
    PthreadTask* eventchecker;
    VideoDPMS* dpmsSafe;
    uint_t m_uiImages;
    avm::vector<CImage*> sflist;
    const subtitle_line_t* m_pSublineRef;
    subtitle_line_t* m_pSubline;
    char* charset;
#ifdef HAVE_ICONV
    XFontSet font;
    char *i18nfileencoding;
    char *i18ndisplayencoding;
    bool i18ncodeconvert;
#else
    XFontStruct* font;
#endif
#ifdef HAVE_LIBXFT
    XftDraw *xftdraw;
    XftFont *xftfont;
    XftColor *xftcolor;
#endif
    Uint32 sdl_systems;

    bool dga;
    bool m_bDirty;
    bool m_bResizeEnabled;
    bool m_bSubRefresh;

public:
    static const int SUBTITLE_SPACE = 3;
    static int s_iTrickNvidia;

    FullscreenRenderer(PlayerWidget* pw, Display* _dpy,
		       int width, int height, bool subt = false)
	:VideoRendererWithLock(width, height, subt),
	dpy(0), xgc(0), m_pSubRenderer(0), screen(0),
        m_pSdlgRtConfig(0),
	max_w(0), max_h(0), image(0),
	convbuf(0), eventchecker(0), dpmsSafe(0),
        m_uiImages(0), m_pSublineRef(0), m_pSubline(0),
	charset(0), font(0),
#ifdef HAVE_LIBXFT
	xftdraw(0), xftfont(0),	xftcolor(0),
#endif
        sdl_systems(0),
	dga(false), m_bDirty(false), m_bResizeEnabled(true),
        m_bSubRefresh(false)
    {
	m_pPw = pw;
        m_Zoom.w = 0;
	//avm_setenv("SDL_WINDOWID", "0x200000e", 1);
	int found = False;
	char *s = setlocale(LC_CTYPE, "");
	if (!s)
	    AVM_WRITE("renderer", "Warning: Locale not supported by C library\n");
	else
	{
	    if (!XSupportsLocale())
	    {
		AVM_WRITE("renderer", "warning: Locale not supported by Xlib\n");
                setlocale(LC_CTYPE, "C");
	    }
            charset = strdup(nl_langinfo(CODESET));
	}

#if _SDL_VER < 1103
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
	atexit(SDL_Quit);
#else
	Uint32 subsystem_init = SDL_WasInit(SDL_INIT_EVERYTHING);

	if (!(subsystem_init & SDL_INIT_VIDEO))
	{
	    if (subsystem_init == 0)
	    {
		SDL_Init(SDL_INIT_NOPARACHUTE);
		atexit(SDL_Quit);
	    }

	    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
		throw FATAL("Failed to init SDL_VIDEO");
            sdl_systems |= SDL_INIT_VIDEO;
	}
#endif
	//fs = SDL_RESIZABLE | SDL_SWSURFACE | SDL_HWACCEL | SDL_ANYFORMAT;
	fs = SDL_HWSURFACE | SDL_HWACCEL | SDL_ANYFORMAT;
#if _SDL_VER > 1103
	fs |= SDL_RESIZABLE;

	//fs |= SDL_DOUBLEBUF;
	//fs |= SDL_ASYNCBLIT;
	// Async blit makes this a bit faster - however we prefer
	// to know when the image is drawn
	// - Async uses XFlush, Sync calls XSync
	// using double buffering would be great
        // but for some reason it doesn't work for YUV
	char vname[100];
	SDL_VideoDriverName(vname, sizeof(vname));
	AVM_WRITE("renderer", "SDL video driver: %s\n", vname);
#endif
	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	AVM_WRITE("renderer", 1, "VideoInfo: %s  %s  %s  %s  %s -  %s  %s  %s  -  %s   vmem: %d    colorkey: 0x%x  alpha: 0x%x\n",
	       (vi->hw_available) ? "hw available" : "",
	       (vi->wm_available) ? "wm_available" : "",
	       (vi->blit_hw) ? "blit_hw" : "",
	       (vi->blit_hw_CC) ? "blit_hw_CC" : "",
	       (vi->blit_hw_A) ? "blit_hw_A" : "",
	       (vi->blit_sw) ? "blit_sw" : "",
	       (vi->blit_sw_CC) ? "blit_sw_CC" : "",
	       (vi->blit_sw_A) ? "blit_sw_A" : "",
	       (vi->blit_fill) ? "blit_fill" : "",
	       vi->video_mem,

	       vi->vfmt->colorkey,
	       vi->vfmt->alpha
	      );

        int dim_w, dim_h;
        pic_w = -1;
	if (getenv("SDL_VIDEODRIVER") != NULL
	    && strcmp(getenv ("SDL_VIDEODRIVER"), "dga") == 0) {
            const char* px = getenv("AVIPLAY_DGA_WIDTH");
	    const char* py = getenv("AVIPLAY_DGA_HEIGHT");
	    float ratio_x = (px == NULL) ? 800. : atof(px);
	    float ratio_y = (py == NULL) ? 600. : atof(py);
	    float ratio;
	    ratio_x = ratio_x / m_w;
	    ratio_y = ratio_y / m_h;
	    ratio = (ratio_x < ratio_y) ? ratio_x : ratio_y;

	    dim_w = width;//(int) rint(ratio * m_w);
	    dim_h = height;//(int) rint(ratio * m_h);
	}
	else
	{
	    dim_w = width;
	    dim_h = height;
	}

	/*int newbpp = SDL_VideoModeOK(dim_w, dim_h, GetPhysicalDepth(_dpy), fs);
	  if (!newbpp)
	    throw FATAL("Failed to set up video mode");
	 */

	doResize(dim_w, dim_h);
	if (dga)
            m_bResizeEnabled = false;
	// doResize already creates screen = SDL_SetVideoMode(dim_w, dim_h, 0, fs);

	if (!screen)
	    throw FATAL("Failed to set up video mode");

	try
	{
	    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	    bit_depth = screen->format->BitsPerPixel;
	    if (bit_depth == 16 && screen->format->Rmask == 0x7c00)
		bit_depth = 15;

	    bpp = (bit_depth + 7) / 8;

	    if (!dga)
		dpy = _dpy;

	    if (dpy)
	    {
                m_Mouse.Open();
		nvidiaCheck(); // hack for TNT2 and bad NVidia driver

		dpmsSafe = new VideoDPMS(dpy);
		keyrepev.type = 0;

#ifdef HAVE_LIBXXF86VM
		XF86VidModeModeInfo** modesinfo = 0;
		int lines;
		Bool r = XF86VidModeGetAllModeLines(dpy, DefaultScreen(dpy),
						    &lines, &modesinfo);
		if (r && modesinfo)
		{
		    for (int i = 0; modesinfo[i] && i < lines; i++)
		    {
			VideoMode vm;
			char b[100];
			vm.width = modesinfo[i]->hdisplay;
			vm.height = modesinfo[i]->vdisplay;
			// some systems provides zeroes here... ???
                        // ATI Radeon VE 32MB @ 1280x1024 some user
			vm.freq = (modesinfo[i]->htotal && modesinfo[i]->vtotal)
			    ? (modesinfo[i]->dotclock * 1000
			       / modesinfo[i]->htotal
			       / (double) modesinfo[i]->vtotal) : 0;
			//calculate refresh frequency
			sprintf(b, (vm.freq) ? "%d x %d  %dHz" : "%d x %d",
				vm.width, vm.height, (int)(vm.freq + 0.5));
			vm.name = b;
			modes.push_back(vm);
		    }
		    XFree(modesinfo);
		}
		else
		    AVM_WRITE("renderer", "Can't resolve video modes...\n");
#else
		// Get available fullscreen/hardware modes
                // not call to FREE for smodes!!
		SDL_Rect **smodes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

		// Check is there are any modes available
		if (smodes != (SDL_Rect **)0)
		{
		    /* Check if our resolution is restricted */
		    if (smodes != (SDL_Rect **)-1)
		    {
			for (int i = 0; smodes[i]; i++)
			{
			    VideoMode vm;
			    char b[100];
			    vm.width = smodes[i]->w;
			    vm.height = smodes[i]->h;
			    sprintf(b, "%d x %d", vm.width, vm.height);
			    vm.name = b;
			    modes.push_back(vm);
			}
		    }
		    else
			AVM_WRITE("renderer", "All resolutions available.\n");
		}
                else
		    AVM_WRITE("renderer", "Can't resolve video modes...\n");
#endif
		SDL_VERSION(&info.version);
		SDL_GetWMInfo(&info);
		dpy = info.info.x11.display;
		Window win = info.info.x11.window;
		XGCValues gcv;
		gcv.graphics_exposures = False;
		xgc = XCreateGC(dpy, win, GCGraphicsExposures, &gcv);
#if 0
#ifdef HAVE_FREETYPE
		m_pSubRenderer = new SubRenderer_Ft();
#endif
#ifdef HAVE_XFT
		if (!m_pSubRenderer)
		    m_pSubRenderer = new SubRenderer_Xft(dpy, win);
#endif
		if (!m_pSubRenderer)
		    m_pSubRenderer = new SubRenderer_X11(dpy, win);
#endif
		SetCaption("AviPlayer: M/F max/fullscreen", 0);
	    }
	    AVM_WRITE("renderer", "created surface: %dx%d %d bits\n", pic_w, pic_h, bit_depth);

#if _SDL_VER > 1104
	    m_pSdlgRtConfig = new SDLGRtConfig(this);
#endif
	    eventchecker = new PthreadTask(0, eventThread, this);
	}
	catch(...)
	{
#if _SDL_VER > 1102
	    SDL_QuitSubSystem(sdl_systems);
#endif
	    throw;
	}
    }
    virtual ~FullscreenRenderer()
    {
	m_bQuit = true;
        emutex.Lock();
        econd.Broadcast();
        emutex.Unlock();
        delete m_pSdlgRtConfig;
	Lock();
#if _SDL_VER > 1102
	if (screen->flags & SDL_FULLSCREEN)
	    SDL_WM_ToggleFullScreen(screen);
#endif
        delete eventchecker;
	delete[] convbuf;
	m_Mouse.Close();

	if (image)
	    image->Release();

	while (sflist.size() > 0)
	{
            //printf("SFLIST FREE  %p\n", sflist.back()->GetUserData());
	    SDL_FreeSurface((SDL_Surface*) sflist.back()->GetUserData());
            sflist.pop_back();
	}

	if (m_pSubline)
            subtitle_line_free(m_pSubline);

	freeFont();
        if (xgc)
	    XFreeGC(dpy, xgc);
	if (charset)
            free(charset);
	//if (screen) SDL_FreeSurface(screen);
	delete dpmsSafe;
        Unlock();
#if _SDL_VER > 1102
	SDL_QuitSubSystem(sdl_systems);
#else
        SDL_Quit();
#endif
    }
    virtual int Set(...)
    {
	va_list args;
	Property prop;

	va_start(args, this);

	while ((prop = (Property) va_arg(args, int)) != 0)
	{

	}
	va_end(args);
	return 0;
    }
    virtual int Get(...) const
    {
	va_list args;
	Property prop;

	//printf("Sizeof %d\n", sizeof(Property));
	va_start(args, this);

	va_end(args);

	return 0;
    }
    virtual IRtConfig* GetRtConfig() const { return m_pSdlgRtConfig; }
    virtual uint_t GetImages() const { return m_uiImages; }
    virtual CImage* ImageAlloc(const BITMAPINFOHEADER& bh, uint_t idx, uint_t aling)
    {
	// for direct rendering bitdepth must match!
        m_bResizeEnabled = false; // not allowed for x11 resizing
	BitmapInfo bi(m_w, m_h, bit_depth);

	//printf("--ImageAlloc\n"); BitmapInfo(bh).Print();
	//printf("--ImageAllocated\n"); bi.Print();
	if (!(bi == BitmapInfo(bh)))
            return 0;

	SDL_Surface* sf = 0;

	Lock();
	while ((unsigned)idx >= sflist.size())
	{
	    if (idx == 0)
		sf = screen;
	    else if (idx > 10000)  // disabled
	    {
		SDL_Surface* cs = SDL_GetVideoSurface();
		sf = SDL_CreateRGBSurface(SDL_HWSURFACE,
					  cs->w, cs->h,
					  cs->format->BitsPerPixel,
					  cs->format->Rmask,
					  cs->format->Gmask,
					  cs->format->Bmask,
					  cs->format->Amask);
                //printf("SDL SURFACE %d %d\n", sf->w, sf->h);
	    }

	    if (!sf)
                break;

	    CImage* ci = new CImage(&bi, (uint8_t*) sf->pixels, false);
	    ci->SetUserData(sf);
            ci->SetAllocator(this);
	    sflist.push_back(ci);
	}
	Unlock();
	if (!sf)
            return 0;

	if (m_uiImages <= idx)
            m_uiImages = idx + 1;
	return sflist[idx];
    }
    virtual void ReleaseImages() { m_uiImages = 0; }
    virtual int Draw(const CImage* data)
    {
	// we have to prevent resizing of screen->pixels
#if 0
	AVM_WRITE("renderer", 1, "ScreenInfo:  colorkey: 0x%x  alpha: 0x%x\n",
		  screen->format->colorkey,
		  screen->format->alpha);
	char* tmp_array=new char[m_w*m_h];
	char* tmp_array2=new char[m_w*m_h];
	uint_t t3=localcount();
    	memcpy(tmp_array2, tmp_array, m_w*m_h);
	uint_t t4=localcount();
	SDL_LockSurface(screen);
	uint_t t1=localcount();
    	memcpy(screen->pixels, tmp_array, m_w*m_h);
	uint_t t2=localcount();
	SDL_UnlockSurface(screen);
	delete[] tmp_array;
	delete[] tmp_array2;
	AVM_WRITE("Pixels: %f Mb/s System: %f Mb/s\n",
	    (m_w*m_h/1048576.)/((t2-t1)/freq/1000.),
	    (m_w*m_h/1048576.)/((t4-t3)/freq/1000.)
	    );
#endif
	//if (TryLock() != 0) return -1;
	// if we do not want to block our renderer while
        // keyboard event processing tries to lock this system
        Locker locker(m_Mutex);

	if (!data)
	{
	    if (!image)
		return -1;

	    data = image;
	}

	data->AddRef();
	if (image)
	    image->Release();
	image = data;

	m_lLastDrawStamp = longcount();
	char* dest = (char*) screen->pixels;
	if (pic_w != m_w || pic_h != m_h || dga)
	{
            if (!convbuf)
		convbuf = new char[m_w * m_h * bpp];
	    dest = convbuf;
	}

	SDL_LockSurface(screen);

	if (!data->GetUserData() || !data->Format() != bit_depth)
	{
	    int w = m_w;
	    int h = m_h;
	    if (m_Zoom.w)
	    {
		w = m_Zoom.w;
		h = m_Zoom.h;
		//printf("%d  %d   %d  %d\n", pic_w, w, pic_h, h);
                if (pic_w == w && pic_h == h)
		    dest = (char*) screen->pixels;
	    }

	    // make a local non-const copy of data
            // so we could set Zoom
	    const uint8_t* p[CIMAGE_MAX_PLANES] = {
		data->Data(0), data->Data(1), data->Data(2)
	    };
	    int s[CIMAGE_MAX_PLANES] = {
		data->Stride(0), data->Stride(1), data->Stride(2)
	    };
	    CImage datah(data->GetFmt(), p, s, false);

	    if (m_Zoom.w)
		datah.SetWindow(m_Zoom.x, m_Zoom.y, m_Zoom.w, m_Zoom.h);

	    BitmapInfo bi(w, h, bit_depth);
	    //data->GetFmt()->Print();
	    //bi.Print();
	    // convert image into either buffer or
	    // directly into the screen surface in case we do not need
	    // more scaling
	    //printf("SCREEN %d\n", screen->pitch);
	    CImage ci(&bi, (const uint8_t*) dest, false);
	    ci.Convert(&datah);
	    //printf("COVERT  %d -> %d      %d,%d   0x%08x\n", data->GetFmt()->Bpp(), bit_depth,
	    //data->GetFmt()->biHeight, bi.biHeight, data->GetFmt()->biCompression);
	    m_bResizeEnabled = true;
	}

	if (dest == convbuf)
	{
	    //printf("Zoom  %d  %d  %d  h: %d  %d\n", pic_w, screen->pitch, screen->pitch / (bit_depth / 8), pic_h, m_h);
	    // scaling is necessary
	    zoom((uint16_t*)screen->pixels, (const uint16_t*)dest,
		 screen->pitch / (bit_depth / 8),
		 //pic_w,
		 pic_h,
		 m_w, m_h, bit_depth);

	    //for (int i = 0; i < screen->h; i++) for (int j = 0; j < 50; j++)
	    //    ((char*)screen->pixels)[i * screen->pitch + j] = i * j;
	}

	SDL_UnlockSurface(screen);

	m_bDirty = true;
	return 0;
    }
    virtual int Sync()
    {
	if (m_bDirty && !dga)
	{
	    Lock();
	    m_bDirty = false;

	    //SDL_Flip(screen);
	    // equals for non double buffered surface
            //printf("Update %d  %d  %d  %d\n",cliprect.x, cliprect.x,cliprect.w, cliprect.h);
	    SDL_UpdateRect(screen, cliprect.x, cliprect.x,
			   cliprect.w, cliprect.h);
	    Unlock();
	}

	//printf("FSMOD %x  %d\n", screen->flags, screen->flags & SDL_FULLSCREEN);
        emutex.Lock();
        econd.Broadcast();
        emutex.Unlock();
	return 0;
    }
    virtual int ToggleFullscreen(bool maximize = false)
    {
	if (dga)
	    return -1;
	Lock();

	if (!(screen->flags & SDL_FULLSCREEN))
	{
	    m_Mouse.Store();
	    if (maximize && m_bResizeEnabled)
	    {
		int w = max_w = pic_w; // set some defaults
		int h = pic_h + m_sub;
		max_h = pic_h;
		GetModeLine(w, h);
		//printf("GETMODELINE  %dx%d   %dx%d\n", w, h, pic_w, pic_h);
		float ratio_x = (float) w / pic_w;
		float ratio_y = (float) (h - m_sub) / pic_h;
		float ratio = (ratio_x < ratio_y) ? ratio_x : ratio_y;
		w = (int) rint(ratio * pic_w);
		h = (int) rint(ratio * pic_h);
		if (pic_w != w || pic_h != h)
		{
		    // FIXME - needs to flush XQueue here to be able to move
		    // this window before Toggle occures
		    if (doResize(w, h) < 0)
			max_w = max_h = 0;
		}
	    }
	}
#if _SDL_VER > 1102
	SDL_WM_ToggleFullScreen(screen);
	fs = screen->flags & SDL_FULLSCREEN;
#else
	screen->flags ^= SDL_FULLSCREEN;
	fs = screen->flags & SDL_FULLSCREEN;
        if (fs && max_w > 0)
	    XMoveWindow(dpy, info.info.x11.window, 0, 0);
#endif

        // testing mouse ungrab in fullscreen mode in xinerama mode
	//XUngrabPointer(info.info.x11.display, CurrentTime);
	//XUngrabKeyboard(info.info.x11.display, CurrentTime);

	if (!fs)
	{
	    m_Mouse.Restore();
	    if (max_w && max_h)
		doResize(max_w, max_h);
	    max_w = max_h = 0;
	}
	m_Mouse.Off();
	Unlock();
	Refresh();
	return fs;
    }
    virtual int DrawSubtitles(const subtitle_line_t* sl)
    {
	if (!dpy || !xgc)
	    return -1;

	Lock();

	if ((!m_bSubRefresh && subtitle_line_equals(sl, m_pSubline))
	    || (!sl && !m_pSubline) || !m_iFontHeight)
	{
	    Unlock();
	    return 0;
	}
	// prepare line - calc width of each line
	// append each overlap to next line

        m_bSubRefresh = false;
        subtitle_line_t* nsl = (sl) ? subtitle_line_copy(sl) : NULL;
	subtitle_line_free(m_pSubline);
	m_pSubline = nsl;

	//if (m_pSubRenderer) m_pSubRenderer->Draw(m_pSubline);

	Window win = info.info.x11.window;
	GC lxgc = xgc;

	XSetForeground(dpy, lxgc, 0x0);
	XFillRectangle(dpy, win, lxgc, 0, pic_h, pic_w, m_sub);
	XSetForeground(dpy, lxgc, 0xFFFFFFFF);

	if (!m_pSubline) {
	    Unlock();
	    return 0;
	}

	char sub[1000];
        *sub = 0;
	bool copy = true;
	for (int i = 0; i < SUBTITLE_SPACE; i++)
	{
	    if (copy && i < m_pSubline->lines)
	    {
		strncat(sub, m_pSubline->line[i], sizeof(sub) - strlen(sub) - 2);

		if ((m_sub / m_iFontHeight) < m_pSubline->lines)
		{
		    for (int j = 1; j < m_pSubline->lines; j++)
		    {
			// compress all subtitles lines into just one line
			strcat(sub, " ");
			strncat(sub, m_pSubline->line[j], sizeof(sub) - strlen(sub) - 2);
		    }
		    copy = false;
		}
	    }

//	    XDrawString16(dpy, win, lxgc, dim_w/2-6*strlen(sub)/2,
//	                  dim_h-65+20*i,
//	                  (XChar2b*)(short*)QString::fromLocal8Bit(sub).unicode(),
//			  strlen(sub));
	    size_t slen = strlen(sub);
	    if (!slen)
                break;
            int draw_h = pic_h + (i + 1) * m_iFontHeight;
#ifdef HAVE_LIBXFT
	    if (slen && xftfont)
	    {
		size_t clen = slen;
		size_t nlen = slen;
		for (;;)
		{
		    XGlyphInfo extents;
        	    XftTextExtents8(dpy, xftfont, (XftChar8*)sub, clen, &extents);
		    int fm_width_sub = extents.xOff;
		    if (fm_width_sub > pic_w)
		    {
			while (clen > 0 && !isspace(sub[clen - 1]))
			    clen--;
			if (clen > 0 && clen < nlen)
			{
                            nlen = clen;
			    while (clen > 0 && isspace(sub[clen - 1]))
			        clen--;
			    if (clen > 0)
				continue;
			}
			clen = nlen;
		    }
		    XftDrawString8(xftdraw, xftcolor, xftfont,
				   (pic_w - fm_width_sub) / 2,
				   draw_h, (XftChar8*)sub, clen);
		    if (nlen != slen)
		    {
                        slen -= nlen;
			memcpy(sub, sub + nlen, slen);
			sub[slen++] = ' ';
                        sub[slen] = 0;
		    }
		    else
			*sub = 0;
                    break;
		}
	    }
#endif /* HAVE_LIBXFT */
	    if (slen && font)
	    {
#ifdef HAVE_ICONV
		size_t in_size = slen;
		size_t out_size = slen * 6;
		char* out_buffer = (char*)malloc(out_size);
		char* out_p = out_buffer;
		iconv_t icvsts = iconv_open(charset, "UTF-8");
		if (icvsts != (iconv_t)(-1)) {
		    char* in_p = sub;
                    while (in_size > 0 && out_size > 10)
			if ((size_t)(-1) == iconv(icvsts, (ICONV_CONST_CAST char**) &in_p, &in_size, &out_p, &out_size)
			    && --in_size > 0)
			    in_p++; // skip and try next

		    iconv_close(icvsts);
		}
                uint_t out_count = out_p - out_buffer;
		//printf("OUTLEN  %d - %d    %d    (%s)\n", out_size, out_count, slen, charset);
		//for (unsigned i =0; i < out_size / sizeof(wchar_t); i++)
                //    printf("char %d   %d  0x%x\n", i, out_buffer[i], out_buffer[i]);
		int fm_width_sub = XTextWidth(font, out_buffer, out_count);
		XDrawString(dpy, win, lxgc, (pic_w - fm_width_sub) / 2,
			    draw_h, out_buffer, out_count);
		free(out_buffer);
#else /* HAVE_ICONV */
		int fm_width_sub = XTextWidth(font, sub, slen);
		//printf("Width %d, height %d   %s\n", fm_width_sub, m_iFontHeight, sub);
		XDrawString(dpy, win, lxgc, (pic_w - fm_width_sub) / 2,
			    draw_h, sub, slen);
#endif /* HAVE_ICONV */
	    }
	    XFlush(dpy);
	}
	Unlock();
	return 0;
    }
    virtual int Resize(int& new_w, int& new_h)
    {
	if (dga)
            return -1;

	Lock();
	int r = doResize(new_w, new_h);
	//printf("DORESIZE %d %d  %d\n", new_w, new_h, r);
	Unlock();
	if (r == 0)
	    Refresh();
	return r;
    }
    virtual int Zoom(int x, int y, int width, int height)
    {
	if (!m_bResizeEnabled)
            return -1;
        Lock();
	m_Zoom.x = x & ~7;
	m_Zoom.y = y & ~1;
	m_Zoom.w = width = (width + 7) & ~7;
        m_Zoom.h = height = (height + 1) & ~1;
	if (m_Zoom.w > 0 && m_Zoom.h > 0)
	    doResize(width, height);
	else
	    m_Zoom.w = m_Zoom.h = 0;

	for (unsigned i = 0; i < sflist.size(); i++)
	    sflist[i]->SetWindow(m_Zoom.x, m_Zoom.y, m_Zoom.w, m_Zoom.h);
	Unlock();

	return 0;
    }
    virtual int Refresh()
    {
	if (to_float(longcount(), m_lLastDrawStamp) > 0.1)
	{
            // wait until we could really refresh image
	    Lock();
	    subtitle_line_t* sl = m_pSubline;
	    m_pSubline = 0;
	    Unlock();
	    // because of scaling we need to call Draw
	    if (Draw(0) == 0)
	    {
		if (sl)
		    DrawSubtitles(sl);
                Sync();
		return 0;
	    }
	}
	else
            m_bSubRefresh = true;

        return -1;
    }
    virtual int SetCaption(const char* title, const char* icon)
    {
	if (!dpy)
	    return -1;

	Lock();
	SDL_WM_SetCaption(title, icon);
        Unlock();
        return 0;
    }
    virtual int GetPosition(int& x, int& y) const
    {
	if (!dpy)
	    return -1;

	Lock();
	doGetPosition(x, y);
	Unlock();
	//printf("Window position is %d %d   s:%d\n", x, y, s);
        return 0;
    }
    virtual int SetPosition(int new_x, int new_y)
    {
	if (!dpy)
	    return -1;

	Lock();
#if _SDL_VER > 1102
	Window win = info.info.x11.wmwindow;
#else
	Window win = info.info.x11.window;
#endif
	XMoveWindow(dpy, win, new_x, new_y);
	Unlock();
	Refresh();

	return 0;
    }
    virtual int SetFont(const char* subfont)
    {

	if (!dpy)
	    return -1;
	avm::string lf = subfont;
	avm::string::size_type n = lf.find(":qtfont=");
	if (n != avm::string::npos)
	    lf[n] = 0;
	if (!lf.size())
            return 0;
	Lock();
        freeFont();

	n = lf.find("-iso");
	if (n != avm::string::npos)
	{
	    lf[n + 1] = '*';
	    lf[n + 2] = 0;
	}

	Window win = info.info.x11.window;
	//XSetWindowAttributes attr;
	//XWindowAttributes src;
	//XGetWindowAttributes(dpy, info.info.x11.window, &src);
	//attr.event_mask=VisibilityChangeMask;
	//XChangeWindowAttributes(dpy, info.info.x11.wmwindow, CWEventMask, &attr);
	//XChangeWindowAttributes(dpy, info.info.x11.window, CWEventMask, &attr);
	//::wnd=info.info.x11.wmwindow;

	if (lf)
	    AVM_WRITE("renderer", "Loading font: \"%s\"\n", lf.c_str());

	m_iFontHeight = 0;

	//font = XLoadQueryFont(dpy, lf);
#ifdef HAVE_LIBXFT
	xftfont = 0; // only truetype fonts
	if (XftDefaultHasRender(dpy) == 1 && lf)
	{
	    xftcolor = new XftColor;
	    xftcolor->color.red = 0xd700;
	    xftcolor->color.green = 0xdc00;
	    xftcolor->color.blue = 0xed00;
	    xftcolor->color.alpha = 0xffff;
	    xftcolor->pixel = 0xd7dced;

	    avm::string lfn = lf;

#ifdef HAVE_ICONV
	    /* Check Font Name Style
	     Type1. Single Font Style.
	     "-sony-fixed-medium-r-normal--24-230-75-75-c-120-iso8859-1"
	     Type2  Multi Font Style.
	     "-sony-fixed-medium-r-normal--24-230-75-75-c-120-jisx0208.1983-0,-sony-fixed-medium-r-normal--24-230-75-75-c-120-jisx0201.1976-0,-sony-fixed-medium-r-normal--24-230-75-75-c-120-iso8859-1"
	     Type 3  Xft Font Name Style
	     "MS UI Gothic-16"
	     */

	    XftPattern* pat = XftXlfdParse(lf, 0, 1);
	    if (pat)
	    {
		XftResult res;
		XftPattern* match = XftFontMatch(dpy, DefaultScreen(dpy),
						 pat, &res);
		if (match)
		{
		    Bool bIsCore;
		    XftPatternGetBool(match, XFT_CORE, 0, &bIsCore);
		    if (!bIsCore)
		    {
			AVM_WRITE("renderer", "XftFont %p\n", match);
			xftfont = XftFontOpenXlfd(dpy, DefaultScreen(dpy),
						  lfn.c_str());
		    }
		    XftPatternDestroy(match);
		}
		AVM_WRITE("renderer", "XftFont Not Match\n");
		XftPatternDestroy(pat);
	    }
	    else
	    {
		if (lf[0] != '-')
		    xftfont = XftFontOpenName(dpy, DefaultScreen(dpy), lf);
	    }
#else /* HAVE_ICONV */

	    // stupid check for truetype font
	    // FIXME:
	    char* p = strstr(lfn.c_str(), "type-");
	    if (p || strstr(lfn.c_str(), "ttf-"))
	    {
		// the purpose of the folloing code is replace last digit
		// in the fontname:  *-iso8859-2  -> *-iso8859-*
		// it's necessary so the Xft renderer will work correctly
		p = strstr(lfn.c_str(), "iso8859-");
		if (p)
		{
		    p += 9;
		    *p = '*';
		    AVM_WRITE("renderer", "Modified font name for iso8859 support\n");
		}
#if 0
		xftfont = XftFontOpen(dpy, DefaultScreen(dpy),
				      XFT_FAMILY, XftTypeString, "Arial",
				      //XFT_ENCODING, XftTypeString, "iso10646",
				      XFT_ENCODING, XftTypeString, "iso8859-2",
				      XFT_SIZE, XftTypeDouble, 20.0,
				      0);
#endif // just testing family open
		xftfont = XftFontOpenXlfd(dpy, win, lfn.c_str());
	    }
	    //xftfont = XftFontOpenName(d, info.info.x11.window, "verdana:pixelsize=30");
#endif /* HAVE_ICONV */
	}

        int iDescent = 0;
	if (xftfont)
	{
	    m_iFontHeight = xftfont->height;
	    iDescent = xftfont->descent;
	    xftdraw = XftDrawCreate(dpy, win, DefaultVisual(dpy, DefaultScreen(dpy)),
				    DefaultColormap(dpy, DefaultScreen(dpy)));
	    AVM_WRITE("renderer", "Successfully opened Xft font\n");
	}
        else
	{
	    AVM_WRITE("renderer", "Failed to open Xft Font\n");
	}
	if (!xftfont)
#endif // HAVE_LIBXFT

	    if (lf.size())
	    {
		font = XLoadQueryFont(dpy, lf.c_str());
		if (!font)
		    AVM_WRITE("renderer", "Failed to open X11 font\n");
	    }

	if (font)
	{
#ifdef HAVE_ICONV
	    XFontSetExtents *extent = XExtentsOfFontSet(font);
	    m_iFontHeight = extent->max_logical_extent.height;
#else  /* HAVE_ICONV */
	    XSetFont(dpy, xgc, font->fid);
	    m_iFontHeight = font->max_bounds.ascent + font->max_bounds.descent;
	    iDescent = font->max_bounds.descent;
	    //printf("Max bounds: %d, %d\n",
	    //       _font->max_bounds.ascent, _font->max_bounds.descent);
#endif /* HAVE_ICONV */
	}

	m_sub = m_iFontHeight * SUBTITLE_SPACE + iDescent;
	//printf("MSUB %d  %d\n", m_sub, m_iFontHeight);
	SDL_Surface* s = SDL_SetVideoMode(pic_w, pic_h + m_sub, 0, fs);
        if (s)
	    screen = s;
	//printf("Max bounds: %d, %d\n",
	//       _font->max_bounds.ascent, _font->max_bounds.descent);
	Unlock();
        return 0;
    }

    void GetModeLine(int& w, int& h)
    {
	// method to detect screen resolution in pixels
	// e.g. when user switches resolution with Ctrl Alt '+'
	if (!dpy)
	    return;
	// this could be called either under SDL or
	// within SDL context - thus open/close new connectins
	// for this query - not very efficient but safe
        // and avoids mixture of Xevents with older XServers
        Display* d = XOpenDisplay(0);
#ifdef HAVE_LIBXXF86VM
	int unused;
	XF86VidModeModeLine vidmode;
	XF86VidModeGetModeLine(d, DefaultScreen(d),
			       &unused, &vidmode);
	w = vidmode.hdisplay;
	h = vidmode.vdisplay;
#else
	// this will return just size of the whole screen
	// and I don't know other way then XF86VidMode
	// to read current screen size
	w = DisplayWidth(d, DefaultScreen(d));
	h = DisplayHeight(d, DefaultScreen(d));
#endif // HAVE_LIBXXF86VM
        XCloseDisplay(d);
    }
#if 1
    virtual int Lock() const
    {
	int r = m_Mutex.Lock();
        if (!dga)
	    info.info.x11.lock_func();
        return r;
    }
    virtual int TryLock() const
    {
	int r = m_Mutex.TryLock();
	if (r == 0)
	{
	    if (!dga)
		info.info.x11.lock_func();
	}
	return r;
    }
    virtual int Unlock() const
    {
	if (!dga)
	    info.info.x11.unlock_func();
	return m_Mutex.Unlock();
    }
#endif
    virtual int processEvent()
    {
	/**
	 *
	 * This function processes events received by SDL window.
	 *
	 */
	static const char* const event_names[]=
	{
	    "SDL_NOEVENT",
	    "SDL_ACTIVEEVENT",
	    "SDL_KEYDOWN",
	    "SDL_KEYUP",
	    "SDL_MOUSEMOTION",
	    "SDL_MOUSEBUTTONDOWN",
	    "SDL_MOUSEBUTTONUP",
	    "SDL_JOYAXISMOTION",
	    "SDL_JOYBALLMOTION",
	    "SDL_JOYHATMOTION",
	    "SDL_JOYBUTTONDOWN",
	    "SDL_JOYBUTTONUP",
	    "SDL_QUIT",
	    "SDL_SYSWMEVENT",
	    "SDL_EVENT_RESERVEDA",
	    "SDL_EVENT_RESERVEDB",
	    "SDL_VIDEORESIZE",
	    "SDL_VIDEOEXPOSE",
	    "SDL_EVENT_RESERVED2",
	    "SDL_EVENT_RESERVED3",
	    "SDL_EVENT_RESERVED4",
	    "SDL_EVENT_RESERVED5",
	    "SDL_EVENT_RESERVED6",
	    "SDL_EVENT_RESERVED7"
	};

        int r = 0;
	SDL_Event event;
	if (TryLock() == 0)
	{
	    m_Mouse.Process(); // MouseOn/Off
	    r = SDL_PollEvent(&event);
	    Unlock();
	}
	if (!r || !m_pPw)
	{
	    // in case there is no incomming event
            // check if there is some 'autorepeated' event
	    if (m_pPw && keyrepev.type == SDL_USEREVENT)
	    {
		// small initial resistance after keypress
		if (m_iAuto++ > 4)
		{
		    switch (keyrepev.key.keysym.sym)
		    {
		    case SDLK_F1:
		    case SDLK_F2:
		    case SDLK_F3:
		    case SDLK_F4:
		    case SDLK_F5:
		    case SDLK_F6:
		    case SDLK_F7:
		    case SDLK_F8:
		    case SDLK_F9:
		    case SDLK_F10:
		    case SDLK_F11:
		    case SDLK_F12:
		    case SDLK_F13:
		    case SDLK_F14:
		    case SDLK_F15:
			m_iAuto = 5;
			//printf("SLOW\n");
		    default:
			m_pPw->PW_key_func(keyrepev.key.keysym.sym, keyrepev.key.keysym.mod);
			if (m_iAuto >= 20)
			{
			    m_iAuto = 20;
                            return 0; // without wait
			}
		    }
		}
	    }

	    emutex.Lock();
	    econd.Wait(emutex, 0.1 / m_iAuto);
	    emutex.Unlock();

	    return 0;
	}
	//printf("EvenType: %d  %s\n", event.type, event_names[event.type]);
	if (event.type == SDL_KEYUP)
	{
	    //cout << "Key " << (int) event.key.keysym.sym << " mod: "
	    //    << event.key.keysym.mod << endl;
	    keyrepev.type = 0; // disable autorepeat
	    m_iAuto = 1;
	}
	else if (event.type == SDL_KEYDOWN && keyrepev.type == 0)
	{
	    bool autorep = false;
	    switch (event.key.keysym.sym)
	    {
	    case SDLK_RETURN:
		autorep = true;
		if (!(event.key.keysym.mod & (KMOD_ALT | KMOD_META)))
		    break;
		autorep = false;
		// fall through - alt + enter
	    case SDLK_ESCAPE:
	    case SDLK_f:
		m_pPw->PW_fullscreen();
		return 1;
	    case SDLK_m:
		m_pPw->PW_maximize_func();
		return 1;
	    case SDLK_x:
		m_pPw->PW_stop_func();
		return 1;
	    case SDLK_v:
		m_pPw->PW_play_func();
		return 1;
	    case SDLK_q:
		m_pPw->PW_quit_func();
		return 1;
	    case SDLK_p:
	    case SDLK_c:
	    case SDLK_SPACE:
		m_pPw->PW_pause_func();
		return 1;
		//break;
	    case SDLK_F1:
	    case SDLK_F2:
	    case SDLK_F3:
	    case SDLK_F4:
	    case SDLK_F5:
	    case SDLK_F6:
	    case SDLK_F7:
	    case SDLK_F8:
	    case SDLK_F9:
	    case SDLK_F10:
	    case SDLK_F11:
	    case SDLK_F12:
	    case SDLK_F13:
	    case SDLK_F14:
	    case SDLK_F15:
	    case SDLK_BACKSPACE:
	    case SDLK_UP:
	    case SDLK_DOWN:
	    case SDLK_LEFT:
	    case SDLK_RIGHT:
	    case SDLK_PAGEUP:
	    case SDLK_PAGEDOWN:
	    case SDLK_a:
	    case SDLK_z:
		autorep = true;
                break;
	    case SDLK_LEFTBRACKET:
	    case SDLK_RIGHTBRACKET:
                break;
	    default:
		break;
	    }

	    //printf("KEYFUNC %d\n",event.key.keysym.sym);
	    // passing all keys
	    m_pPw->PW_key_func(event.key.keysym.sym, event.key.keysym.mod);
	    if (autorep && !keyrepev.type)
	    {
		memcpy(&keyrepev, &event, sizeof(keyrepev));
		keyrepev.type = SDL_USEREVENT;
	    }
	}
#if _SDL_VER > 1102
	else if (event.type == SDL_VIDEORESIZE)
	{
	    //printf("RESIZE EVENT %d %d\n", event.resize.w, event.resize.h);
	    m_pPw->PW_resize(event.resize.w, event.resize.h - m_sub);
	}
#endif
	else if (event.type == SDL_MOUSEBUTTONDOWN)
	{
	    if (!(SDL_GetModState()
		  & (KMOD_CTRL | KMOD_SHIFT | KMOD_ALT | KMOD_META)))
	    {
		switch (event.button.button)
		{
		case SDL_BUTTON_RIGHT:
		    if (SDL_GetVideoSurface()->flags & SDL_FULLSCREEN)
			m_pPw->PW_fullscreen();

		    // fix race for Pointer grab
#if defined(HAVE_LIBQT) && (HAVE_LIBQT < 302 || HAVE_LIBQT > 303 )
		    Lock();
		    XUngrabPointer(info.info.x11.display, CurrentTime);
		    XUngrabKeyboard(info.info.x11.display, CurrentTime);
		    Unlock();
#endif
		    m_pPw->PW_menu_slot();
		    break;
		case SDL_BUTTON_MIDDLE:
		    m_pPw->PW_middle_button();
		    break;
		case SDL_BUTTON_LEFT:
		    m_pPw->PW_pause_func();
		    break;
		case SDL_BUTTON_WHEELUP:
		    m_pPw->PW_key_func(SDLK_PAGEUP, 0);
		    break;
		case SDL_BUTTON_WHEELDOWN:
		    m_pPw->PW_key_func(SDLK_PAGEDOWN, 0);
		    break;
		}
	    }
	}
#ifdef SDL_VIDEOEXPOSE
	else if (event.type == SDL_VIDEOEXPOSE)
	{
	    // we are not receiving this event so far !!
	    AVM_WRITE("renderer", "SDL_EXPOSE arrived\n");
	}
#endif
	else if (event.type == SDL_ACTIVEEVENT)
	{
	    if (event.active.state == SDL_APPINPUTFOCUS)
	    {
		//m_bKeyboardOn = (event.active.gain) ? true : false;
		m_pPw->PW_refresh();
	    }
            // FIXME - check position change
	}
	else if (event.type == SDL_VIDEOEXPOSE)
	{
	    m_pPw->PW_refresh();
	}
	else if (event.type == SDL_MOUSEMOTION)
	{
            m_Mouse.Set(event.motion.x, event.motion.y);
	}
	else if (event.type == SDL_QUIT)
	{
	    m_pPw->PW_quit_func();
	}

	return 1; // try once more again
    }

protected:
    virtual void doGetPosition(int& x, int& y) const
    {
	XWindowAttributes xwa;
	Window win;
#if _SDL_VER > 1102
	win = info.info.x11.wmwindow;
#else
	win = info.info.x11.window;
#endif
	Status s = XGetWindowAttributes(info.info.x11.display, win, &xwa);
	XTranslateCoordinates(info.info.x11.display, win,
			      xwa.root, -xwa.border_width, -xwa.border_width,
			      &x, &y, &win);
	//printf("WINDOWS  %ld  %ld  %ld\n", info.info.x11.window, info.info.x11.wmwindow, xwa.root);
    }
    virtual int doResize(int& new_w, int& new_h)
    {
	// allow original size
        if (new_w != m_w)
	    new_w = (new_w + 7) & ~7;
	if (new_h != m_h)
	    // some resizing algorithms requires this for now
	    new_h = (new_h + 1) & ~1;

	if (!m_bResizeEnabled)
	{
	    AVM_WRITE("renderer", "Resize is unsupported for RGB Direct mode!\n");
	    //printf("Disabled %dx%d  %dx%d\n", new_w, new_h, pic_w, pic_h);
	    new_w = m_w;
	    new_h = m_h;
	}

	if (new_w < 8 || new_h < 2 || !m_bResizeEnabled)
	    return -1;

	if (pic_w != new_w || pic_h != new_h)
	{
	    if (image && screen && image->Data() == screen->pixels)
	    {
		AVM_WRITE("renderer", "Resize is unsupported for RGB Direct mode!\n");
		new_w = m_w;
		new_h = m_h;
		return -1;
		CImage* ci = new CImage(image);
		image->Release();
		// replace direct image with a new copy
		image = ci;
	    }
	    cliprect.x = cliprect.y = 0;
	    cliprect.w = pic_w = new_w;
	    cliprect.h = pic_h = new_h;
	    // 0 is intentional -> use current screen depth and color model
	    screen = SDL_SetVideoMode(pic_w, pic_h + m_sub, 0, fs);
	    //printf("SetVideoMode pic_w:    %d x %d   %d\n", pic_w, pic_h, m_sub);
	    delete[] convbuf;
	    convbuf = 0;
	}
	return 0;
    }
private:
    virtual void freeFont()
    {
	if (!dpy)
            return;
#ifdef HAVE_LIBXFT
	if (xftfont)
	    XftFontClose(dpy, xftfont);
        xftfont = 0;
	if (xftdraw)
	    XftDrawDestroy(xftdraw);
	xftdraw = 0;
	if (xftcolor)
	    delete xftcolor;
        xftcolor = 0;
#endif
	if (font)
	    XFreeFont(dpy, font);
        font = 0;
    }
    // private implementation - called with lock
    void nvidiaCheck()
    {
	if (s_iTrickNvidia == -1)
	{
	    int lines;
	    //XExtensionVersion* ext = XGetExtensionVersion(dpy, "NVIDIA-GLX");
	    int n = 0;
	    char **extlist = XListExtensions(dpy, &n);

	    //AVM_WRITE ("renderer", 1, "number of extensions:    %d\n", n);
	    s_iTrickNvidia = 0;
	    if (extlist) {
		int i;
		int opcode, event, error;

		for (i = 0; i < n; i++)
		    if (strcmp("NVIDIA-GLX", extlist[i]) == 0)
		    {
			AVM_WRITE("renderer", 0, "Detected nVidia GLX driver\n");
			AVM_WRITE("renderer", 0, "If you need to clear two lowest lines set shell variable AVIPLAY_NVIDIA_ENABLE\n");
		    }
		/* ??? do not free ??? could Xlib depend on contents being unaltered */
		XFreeExtensionList(extlist);
	    }

	    if (getenv("AVIPLAY_NVIDIA_ENABLE"))
	    {
		s_iTrickNvidia = 2;
		AVM_WRITE("renderer", "nVidia - line clearing hack - forced On\n");
	    }
	    if (getenv("AVIPLAY_NVIDIA_DISABLE"))
	    {
		s_iTrickNvidia = 0;
                AVM_WRITE("renderer", "nVidia - line clearing hack - forced Off\n");
	    }
	    //s_iTrickNvidia = 1;
	}
    }
};

int FullscreenRenderer::s_iTrickNvidia = -1;

#undef __MODULE__
#define __MODULE__ "YUV renderer"

#ifdef HAVE_LIBXV

static const char* xvset = "XV_SET_DEFAULTS";

static const struct {
    const char* atom;
    const char* name;
} xvattrs[] = {
    { "XV_HUE", "Hue",				},
    { "XV_BRIGHTNESS", "Brightness"		},
    { "XV_CONTRAST", "Contrast" 		},
    { "XV_SATURATION", "Saturation" 		},
    { "XV_RED_INTENSITY", "Red intensity"	},
    { "XV_GREEN_INTENSITY", "Green intensity"	},
    { "XV_BLUE_INTENSITY", "Blue intensity"	},
    { "XV_COLORKEY", "Colorkey"			},
    { "XV_DOUBLE_BUFFER", "Double buffering"	},

    /* this shouldn't be exposed to the user
    { "XV_COLOR", "Color",			},
    { "XV_MUTE", "Mute"				},
    { "XV_VOLUME", "Volume"			},
    { "XV_FREQ", "Freq"				},
    { "XV_ENCODING", "Encoding"			},
    { "XV_SET_DEFAULTS", "Set default" 		},
    { "XV_AUTOPAINT_COLORKEY", "Autopaint"     	},
    */
    { 0 }
};

#if 0
static void xv_write_attr(Display* dpy, int xv_port, const char* attr, int value)
{
    if (attr->id == ATTR_ID_NORM || attr->id == ATTR_ID_INPUT) {
	if (attr->id == ATTR_ID_NORM)
	    h->norm  = value;
	if (attr->id == ATTR_ID_INPUT)
	    h->input = value;
	for (i = 0; i < h->encodings; i++) {
	    if (h->enc_map[i].norm  == h->norm &&
		h->enc_map[i].input == h->input) {
		h->enc = i;
		XvSetPortAttribute(dpy,h->vi_port,h->xv_encoding,h->enc);
		break;
	    }
	}
    }
    /* needed for proper timing on the
       "mute - wait - switch - wait - unmute" channel switches */
    //XSync(dpy, False);
}
#endif

static int xv_scan_attrs(avm::vector<AttributeInfo>& vattrs, Display* dpy, int* xv_port)
{
    int have_def = 0;
    unsigned int ver, rev, req, evn, err;
    if (!*xv_port
	&& Success != XvQueryExtension(dpy, &ver, &rev, &req, &evn, &err))
        return 0;

    if (*xv_port == 0)
    {
	unsigned int adaptors;
	XvAdaptorInfo* ai;
	/* check for Xvideo support */
	if (Success != XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &adaptors, &ai))
	{
	    AVM_WRITE("renderer", "Xv: XvQueryAdaptors failed");
	    return 0;
	}
	/* check adaptors */
	for (unsigned i = 0; i < adaptors && *xv_port == 0; i++)
	{
	    if (ai[i].type & XvInputMask && ai[i].type & XvImageMask)
	    {
		for (unsigned p = ai[i].base_id; p < ai[i].base_id + ai[i].num_ports; p++)
		{
		    *xv_port = p;
                    break;
		    XvUngrabPort(dpy, p, CurrentTime);
		    if (!XvGrabPort(dpy, p, CurrentTime)) {
			AVM_WRITE("renderer", "Adaptor %d; format list:\n", i);
			for (unsigned j = 0; j < ai[i].num_formats; j++)
			{
			    AVM_WRITE("renderer", "depth=%d, visual=%ld\n",
				   ai[i].formats[j].depth,
				   ai[i].formats[j].visual_id);
			}
			*xv_port = p;
			// 4616 HLA MISS 1 (az 18)
			break;
		    } else {
			AVM_WRITE("renderer", "Xv: could not grab port %i\n", (int)p);
		    }
		}
	    }
	}
    }
    //printf(" attribute list for port %d\n", xv_port);
    int attributes;
    XvAttribute* at = XvQueryPortAttributes(dpy, *xv_port, &attributes);
    if (at)
    {
	for (int j = 0; j < attributes; j++)
	{
	    const char* nm = 0;//at[j].name;
	    int n = 0;
	    while (xvattrs[n].atom != 0)
	    {
		if (!strcmp(xvattrs[n].atom, xvset))
		    have_def++;
		if (!strcmp(xvattrs[n].atom, at[j].name))
		{
		    nm = xvattrs[n].name;
		    break;
		}
		n++;
	    }
	    if (at[j].flags & XvGettable && at[j].flags & XvSettable)
	    {
		int result;
		Atom atom = XInternAtom(dpy, at[j].name, True);
		XvGetPortAttribute(dpy, *xv_port, atom, &result);
		AVM_WRITE("renderer", 0, "XV attribute: %s"
			  "  %s%s   <%i, %i> = %d\n",
			  at[j].name,
			  (at[j].flags & XvGettable) ? "G" : "",
			  (at[j].flags & XvSettable) ? "S" : "",
			  at[j].min_value, at[j].max_value, result);
		if (nm)
		    vattrs.push_back(AttributeInfo(at[j].name, nm,
						   AttributeInfo::Integer,
						   at[j].min_value,
						   at[j].max_value,
						   result));
	    }
	    //printf("SIZEADD %d\n", vattrs.size());
	}
	XFree(at);
    }
    return have_def;
}

class XvRtConfig : public IRtConfig
{
    VideoRendererWithLock* vr;
    avm::vector<AttributeInfo> vattrs;
    avm::vector<int> vattrsorig;
    Display* dpy;
    int xv_port;
    int have_def;
public:
    XvRtConfig(VideoRendererWithLock* _vr, Display* _dpy, int _xv_port)
	: vr(_vr), dpy(_dpy), xv_port(_xv_port)
    {
	//AVM_WRITE("renderer", "xv: port: %d\n", xv_port);
	//xv_port = 0;
	AVM_WRITE("renderer", "XV port: %d\n", xv_port);
	have_def = xv_scan_attrs(vattrs, dpy, &xv_port);//xv_port);
        vattrsorig.resize(vattrs.size());
	if (have_def)
	    SetValue(xvset, 0);
	for (unsigned i = 0; i < vattrs.size(); i++)
	{
	    int val = RegReadInt("aviplay", vattrs[i].GetName(), vattrs[i].GetDefault());
	    SetValue(vattrs[i].GetName(), val);
	}
	//printf("RETURN %d\n", vattrs.size());
    }
    virtual ~XvRtConfig()
    {
	// restore to defaults or at least those from the begining
	if (have_def)
	    SetValue(xvset, 0);
	else
	    for (unsigned i = 0; i < vattrs.size(); i++)
	    {
                int val = 0;
		GetValue(vattrs[i].GetName(), &val);
		RegWriteInt("aviplay", vattrs[i].GetName(), val);
		SetValue(vattrs[i].GetName(), vattrs[i].GetDefault());
	    }
    }
    // IRtConfig interface
    virtual const avm::vector<AttributeInfo>& GetAttrs() const
    {
	return vattrs;
    }
    virtual int GetValue(const char* attr, int* result) const
    {
        vr->Lock();
	Atom atom = XInternAtom(dpy, attr, True);
	XvGetPortAttribute(dpy, xv_port, atom, result);
	AVM_WRITE("renderer", 1, "xv: get %s:%d %d\n", attr, xv_port, *result);
	vr->Unlock();
        return 0;
    }
    virtual int SetValue(const char* attr, int value)
    {
        vr->Lock();
	Atom atom = XInternAtom(dpy, attr, True);
	XvSetPortAttribute(dpy, xv_port, atom, value);
	AVM_WRITE("renderer", 1, "xv: set %s:%d  atom:%d  value: %d\n", attr, xv_port, atom, value);
	vr->Unlock();
        return 0;
    }
};
#else
class XvRtConfig : public IRtConfig
{
public:
    virtual int GetValue(const char* attr, int* result) const { return 0; }
    virtual int SetValue(const char* attr, int value) { return 0; }
};
#endif

#if _SDL_VER > 1103

/**
 *
 * SDL YUV Renderer
 *
 * currently SDL is able to use only XShm XV extension
 *
 *
 *
 *
 */

class YUVRenderer: public FullscreenRenderer
{
protected:
    SDL_Overlay* m_ov;
    SDL_Overlay* m_zoomov;
    fourcc_t m_fmt;
    avm::vector<CImage*> ovlist;
    XvRtConfig*  m_pXvRtConfig;

public:
    YUVRenderer(PlayerWidget* pw, Display* _dpy,
		int _width, int _height, fourcc_t yuvm_fmt, bool _subtitles = false)
	: FullscreenRenderer(pw, _dpy, _width, _height, _subtitles),
	m_ov(0), m_zoomov(0), m_fmt(yuvm_fmt), m_pXvRtConfig(0)
    {
	if (dga)
	    throw FATAL("Requested DGA driver - YUV not available!");

	Lock(); // event thread is already running

//	fs &= ~SDL_DOUBLEBUF; // doesn't work with HW overlay
//	fs |= SDL_HWSURFACE;
	//screen->flags = fs;
	screen = SDL_SetVideoMode(pic_w, pic_h + m_sub, 0, fs);
#if 0
	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	AVM_WRITE("renderer", 0, "VideoInfo: %s  %s  %s  %s  %s -  %s  %s  %s  -  %s   vmem: %d    colorkey: 0x%x  alpha: 0x%x\n",
	       (vi->hw_available) ? "hw available" : "",
	       (vi->wm_available) ? "wm_available" : "",
	       (vi->blit_hw) ? "blit_hw" : "",
	       (vi->blit_hw_CC) ? "blit_hw_CC" : "",
	       (vi->blit_hw_A) ? "blit_hw_A" : "",
	       (vi->blit_sw) ? "blit_sw" : "",
	       (vi->blit_sw_CC) ? "blit_sw_CC" : "",
	       (vi->blit_sw_A) ? "blit_sw_A" : "",
	       (vi->blit_fill) ? "blit_fill" : "",
	       vi->video_mem,

	       vi->vfmt->colorkey,
	       vi->vfmt->alpha
	      );

	AVM_WRITE("renderer", 0, "ScreenInfo:  colorkey: 0x%x  alpha: 0x%x\n",
	       screen->format->colorkey,
	       screen->format->alpha);
#endif
#if 0
	char* tmp_array=new char[_width*_height*2];
	char* tmp_array2=new char[_width*_height*2];
	uint_t t1=localcount();
	memcpy(tmp_array2, tmp_array, _width*_height*2);
	uint_t t2=localcount();
	AVM_WRITE("Memory->memory copy: %f Mb/s\n",
		(_width*_height*3/2/1048576.)/((t2-t1)/freq/1000.));
	delete[] tmp_array2;
	SDL_Overlay* ovs[200];
	for(int i=0; i<200; i++)
	{
	    ovs[i]=SDL_CreateYUVOverlay(_width, _height, fccYV12, screen);
	    SDL_LockYUVOverlay(ovs[i]);
	    uint_t t1=localcount();
	    memcpy(ovs[i]->pixels[0], tmp_array, _width*_height*3/2);
	    uint_t t2=localcount();
	    SDL_UnlockYUVOverlay(ovs[i]);
	    AVM_WRITE("overlay %d ( %f Mb used ): %f Mb/s\n",
		i, i*_width*_height*3/2/1048576.,
		(_width*_height*3/2/1048576.)/((t2-t1)/freq/1000.));
	}
	for(int i=0; i<200; i++)
	    SDL_FreeYUVOverlay(ovs[i]);
	delete[] tmp_array;
#endif
	//printf("IMGFMT %x  %.4s\n", m_fmt, (char*)&m_fmt);

	BitmapInfo bi(m_w, m_h, m_fmt);
	Unlock();
	CImage* ci = ImageAlloc(bi, 0, 0);
        ci->Release();
	if ((!m_ov)
#if _SDL_VER>1104
    // SDL <=1.1.5 does not have hw_overlay flag
	    || !m_ov->hw_overlay
#endif
	   )
	{
	    const char* errmsg = (!m_ov) ? "Failed to create overlay" :
		"No hardware YUV acceleration detected!";

	    AVM_WRITE("renderer", "%s\n", errmsg);
#if _SDL_VER>1106
	    if  (m_ov && allow_sw_yuv)
	    {
		AVM_WRITE("renderer",
		    "*** Using SDL software YUV emulation ***\n"
                    "  Usually most codecs supports RGB modes - so you may\n"
		    "  achieve better performance with disabled YUV flag\n");

                // do not bother user with Xlib warning messages
		avm_setenv("SDL_VIDEO_YUV_HWACCEL", "0", 1);
	    }
            else
#endif
	    {
		if (m_ov)
		{
		    Lock();
		    AVM_WRITE("renderer", "Your SDL library is too old and doesn't support software YUV emulation - upgrade SDL package!\n");
		    SDL_FreeYUVOverlay(m_ov);
		    Unlock();
		}
		throw FATAL(errmsg);
	    }
	}
	else
	{
	    delete m_pSdlgRtConfig;
	    m_pSdlgRtConfig = 0;
	}

	AVM_WRITE("renderer", 0, "created overlay: %dx%d %s\n",
		  _width, _height, avm_img_format_name(m_fmt));

	//Zoom(50, 50, 200, 200);
#ifdef HAVE_LIBXV
	if (!m_pSdlgRtConfig && m_ov->hw_overlay)
	{
	    // hack - we want to get to the grabbed Xv port
	    struct hack_privdata { int port; };
	    int port =  ((struct hack_privdata*) m_ov->hwdata)->port;
	    m_pXvRtConfig = new XvRtConfig(this, dpy, port);
	}
#endif
    }
    ~YUVRenderer()
    {
	delete m_pXvRtConfig;
        ReleaseImages();
        Lock();
	if (m_zoomov)
	    SDL_FreeYUVOverlay(m_zoomov);
	if (m_ov)
            SDL_FreeYUVOverlay(m_ov);
	Unlock();
    }
    virtual IRtConfig* GetRtConfig() const { return m_pXvRtConfig ? (IRtConfig*) m_pXvRtConfig : (IRtConfig*) m_pSdlgRtConfig; }
    virtual CImage* ImageAlloc(const BITMAPINFOHEADER& bh, uint_t idx, uint_t align)
    {
        align = 16; //HACK FIXME
	//printf("IA 0x%x==0x%x  %d %d   %d %d   align:%d  idx:%d-%d\n",bh.biCompression, m_fmt, m_w, bh.biWidth, m_h, bh.biHeight, align, idx, ovlist.size());
	if (m_w != (int)bh.biWidth || m_h != (int)-bh.biHeight || bh.biCompression <= 32)
            return 0;

        Lock();
	while ((unsigned)idx >= ovlist.size())
	{
	    // ok here is little trick
	    // we need to use aligned size - but for display
	    // we do not want to show these extra pixels
	    // so here we will modify the size a little bit
	    int mh = m_h;
	    switch (bh.biCompression) {
	    case fccI420:
	    case fccYV12:
		if (align > 0)
		    mh = ((m_h + align - 1) & ~(align - 1));
		break;
	    }

	    // fails! if (fmt == fccI420) fmt = SDL_IYUV_OVERLAY;
	    // make the image rounded for 16
            // width aligned by 8x - fix for my broken i855 YUV driver
	    SDL_Overlay* o = SDL_CreateYUVOverlay((m_w + 7) & ~7, mh, bh.biCompression, screen);
	    if (!o) {
		AVM_WRITE("renderer", "Failed to create SDL Overlay: %dx%d\n", m_w, mh);
		break;
	    }

	    //printf("M_H:%d MH:%d H:%d  W:%d M_W:%d\n", m_h, mh, o->h, o->w, m_w);
	    if (m_h < mh)
	    {
#ifdef HAVE_LIBXV
		struct private_yuvhwdata { // copy from SDL
		    int port;
		    XShmSegmentInfo yuvshm;
		    XvImage *image;
		};
		//((struct private_yuvhwdata*) o->hwdata)->image->height = m_h;
#endif
                // now modify the size
		o->h = m_h - 1; // HACK!!
	    }

	    BitmapInfo bi(m_w, mh, bh.biCompression);
	    // CHECKME  return SDL1.1.3 compatibility
            const uint8_t* planes[3];
	    int stride[3];
            planes[0] = o->pixels[0];
	    stride[0] = o->pitches[0];
	    switch (bh.biCompression) {
	    case fccI420:
	    case fccYV12:
		planes[1] = o->pixels[1];
		planes[2] = o->pixels[2];
		stride[1] = o->pitches[1];
		stride[2] = o->pitches[2];
		break;
	    default:
		planes[1] = planes[2] = 0;
		stride[1] = stride[2] = 0;
	    }

	    CImage* ci = new CImage(&bi, planes, stride, false);
	    //printf("NEW DATA %p  %x\n", ci, ci->Format());
	    //AVM_WRITE("AFTERORIMGALO  %d  %d   %p\n", idx, ovlist.size(), ovlist[idx]->pixels);
	    ci->SetUserData(o);
	    ci->SetAllocator(this);
	    ci->Clear();
	    ovlist.push_back(ci);
	    if (!m_ov)
	    {
		SDL_Rect rect = { 0, 0, o->w, o->h };
		// activate overlay so YUV preferencies
		// for brightness etc. can be set
		SDL_DisplayYUVOverlay(o, &rect);
		m_ov = o;
	    }
            else if (idx == 0 && o)
	    {
                // ok let's make it as a leading FMT
		SDL_FreeYUVOverlay(m_ov);
		m_ov = o;
		m_fmt = bh.biCompression;
		//printf("NEW GROUP  %x\n", m_fmt);
	    }
	}
	Unlock();
	if (idx >= ovlist.size())
            return 0;
	ovlist[idx]->AddRef();
	if (m_uiImages <= idx)
            m_uiImages = idx + 1;

	return ovlist[idx];
    }
    virtual void ReleaseImages() {
	Lock();
	m_uiImages = 0;
	if (image)
	    image->Release();
	image = 0;
	while (ovlist.size() > 0)
	{
	    SDL_Overlay* o = (SDL_Overlay*) ovlist.back()->GetUserData();
            // leave initially allocate
            if (o != m_ov)
		SDL_FreeYUVOverlay(o);
            ovlist.back()->Release();
            ovlist.pop_back();
	}
        Unlock();
    }
    virtual int Draw(const CImage* data)
    {
	// if we do not want to block our renderer while
	// keyboard event processing tries to lock this system
        // using just normal MUTEX - we are not writing anything to Xfree
	Locker locker(m_Mutex);
	if (!data)
	{
	    if (!image)
		return -1;
	    data = image;
	}

	m_lLastDrawStamp = longcount();

	data->AddRef();
	if (image)
	    image->Release();
	image = data;
        //AVM_WRITE("DATA %x  %x\n", data->GetFmt()->biCompression, m_fmt);
	//printf("ALLOCATOR %p  %p    %x %x   %p\n", data->GetAllocator(),
	//       data->GetUserData(), data->Format(), m_fmt, m_zoomov);
	if (m_zoomov || !data->GetUserData())
	{
	    SDL_Overlay* o = m_ov;
            CImage* datah = 0;
	    if (m_zoomov) {
		o = m_zoomov;
		if (data->m_Window.w != m_Zoom.w)
		{
		    const uint8_t* p[CIMAGE_MAX_PLANES] = {
                        data->Data(0), data->Data(1), data->Data(2)
		    };
		    int s[CIMAGE_MAX_PLANES] = {
			data->Stride(0), data->Stride(1), data->Stride(2)
		    };
		    datah = new CImage(data->GetFmt(), p, s, false);
                    datah->SetWindow(m_Zoom.x, m_Zoom.y, m_Zoom.w, m_Zoom.h);
		}
	    }

	    assert(o != 0);
	    //SDL_LockYUVOverlay(o);
	    BitmapInfo bi(o->w, o->h, m_fmt);
	    //data->GetFmt()->Print();
	    //bi.Print();

	    // CHECKME  return SDL1.0 compatibility
	    int stride[3] = { o->pitches[0], o->pitches[1], o->pitches[2] };
	    CImage ci(&bi, (const uint8_t**) o->pixels, stride, false);
	    //printf("Set  %d %d   %d  %d\n", m_Zoom.x, m_Zoom.y, m_Zoom.w, m_Zoom.h);
	    ci.SetWindow(0, 0, m_Zoom.w, m_Zoom.h);
	    if (datah)
	    {
		ci.Convert(datah);
                datah->Release();
	    }
	    else
		ci.Convert(data);
	    //printf("CONVERT  %p  %x-%x  %p\n", m_zoomov, data->Format(), m_fmt, data->GetUserData());
	    //SDL_UnlockYUVOverlay(o);
	}

	return 0;
    }
    virtual int Sync()
    {
	//printf("SYNC  %d\n", pic_h);
        Lock();
	SDL_Rect rect = { 0, 0, pic_w, pic_h };// + 1;
	SDL_Overlay* o = (m_zoomov) ? m_zoomov : (SDL_Overlay*) image->GetUserData();
	if (!o)
	    o = m_ov;
	//printf("OVERLA %p  h:%d  %d %d\n", o, o->h, pic_w, pic_h);
	assert(o != 0);
	// no surface lock as SDL documentation recommends!
	// the following operation might lock itself in XServer
	if (s_iTrickNvidia > 0)
	{
	    XSetForeground(dpy, xgc, 0x0);
	    XFillRectangle(dpy, info.info.x11.window,
			   xgc, 0, pic_h - s_iTrickNvidia,
			   pic_w, s_iTrickNvidia);
	}
	SDL_DisplayYUVOverlay(o, &rect);
	/*
	 for (int i = 0; i < pic_h + 50; i++)
	 for (int j = 0; j < pic_w; j++)
	 ((short*)screen->pixels)[i * pic_w + j] = 0xffff - j;
	 */
	 //SDL_UpdateRect(screen, rect.x, rect.y, rect.w, rect.h);
	//SDL_UpdateRect(screen, 0, 0, 0, 0);
	Unlock();
	emutex.Lock();
	econd.Broadcast();
	emutex.Unlock();
	return 0;
    }
    virtual int Zoom(int x, int y, int width, int height)
    {
        Lock();
	if (m_zoomov)
	{
	    SDL_FreeYUVOverlay(m_zoomov);
	    m_zoomov = 0;
	}

	m_Zoom.x = x & ~7;
	m_Zoom.y = y & ~1;
	m_Zoom.w = width = (width + 7) & ~7;
	m_Zoom.h = height = (height + 1) & ~1;
	if ((width == m_w && height == m_h)
            || m_Zoom.w <= 0 || m_Zoom.h <= 0)
	    m_Zoom.w = m_Zoom.h = 0; // no cropping needed
        else
	    m_zoomov = SDL_CreateYUVOverlay(m_Zoom.w, m_Zoom.h, m_fmt, screen);
	//printf("%d %d %d %d   %d %d\n", m_Zoom.x, m_Zoom.y, m_Zoom.w, m_Zoom.h, width, height);

	for (unsigned i = 0; i < ovlist.size(); i++)
	    ovlist[i]->SetWindow(m_Zoom.x, m_Zoom.y, m_Zoom.w, m_Zoom.h);
	Unlock();

	return 0;
    }
protected:
    virtual int doResize(int& new_w, int& new_h)
    {
	if (new_w != m_w)
	    new_w = (new_w + 7) & ~7;
	if (new_h != m_h)
	    new_h = (new_h + 1) & ~1;

	if (new_w < 8 || new_h < 2)
	    return -1;

	if (pic_w != new_w || pic_h != new_h)
	{
	    pic_w = new_w;
	    pic_h = new_h;
#if _SDL_VER > 1103
	    fs |= SDL_RESIZABLE;
#endif
	    SDL_SetVideoMode(pic_w, pic_h + m_sub, 0, fs);
	    //printf("SET SCREEN2 = %p   %d,%d,%d\n", s, pic_w, pic_h, m_sub);
	    screen = SDL_GetVideoSurface();
	}

        return 0;
    }
};

#endif // _SDL_VER > 1103

#ifdef HAVE_VIDIX

#undef __MODULE__
#define __MODULE__ "VIDIX renderer"

#define VIDIXCSTR(name) \
    static const char* name = #name

VIDIXCSTR(VIDIX_BRIGHTNESS);
VIDIXCSTR(VIDIX_CONTRAST);
VIDIXCSTR(VIDIX_SATURATION);
VIDIXCSTR(VIDIX_HUE);

class VidixRtConfig : public IRtConfig
{
    VideoRendererWithLock* vr;
    VDL_HANDLE handler;
    mutable vidix_video_eq_t info;
    avm::vector<AttributeInfo> vattrs;
public:
    VidixRtConfig(VideoRendererWithLock* _vr, VDL_HANDLE _handler)
	: vr(_vr), handler(_handler)
    {
	vdlPlaybackGetEq(handler, &info);
	if (info.cap & VEQ_CAP_BRIGHTNESS)
	    vattrs.push_back(AttributeInfo(VIDIX_BRIGHTNESS, "Brightness",
					   AttributeInfo::Integer,
					   -1000, 1000, info.brightness));
        if (info.cap & VEQ_CAP_CONTRAST)
	    vattrs.push_back(AttributeInfo(VIDIX_CONTRAST, "Contrast",
					   AttributeInfo::Integer,
					   -1000, 1000, info.contrast));
        if (info.cap & VEQ_CAP_SATURATION)
	    vattrs.push_back(AttributeInfo(VIDIX_SATURATION, "Saturation",
					   AttributeInfo::Integer,
					   -1000, 1000, info.saturation));
        if (info.cap & VEQ_CAP_HUE)
	    vattrs.push_back(AttributeInfo(VIDIX_HUE, "Hue",
					   AttributeInfo::Integer,
					   -1000, 1000, info.hue));
	for (unsigned i = 0; i < vattrs.size(); i++)
	    SetValue(vattrs[i].GetName(),
		     RegReadInt("aviplay", vattrs[i].GetName(), vattrs[i].GetDefault()));
    }
    virtual ~VidixRtConfig()
    {
	for (unsigned i = 0; i < vattrs.size(); i++)
	{
            int val = 0;
            GetValue(vattrs[i].GetName(), &val);
	    RegWriteInt("aviplay", vattrs[i].GetName(), val);
	    //printf("WRITE  %s  %d\n", vattrs[i].GetName(), val);
            SetValue(vattrs[i].GetName(), vattrs[i].GetDefault());
	}
    }
    // IRtConfig interface
    virtual const avm::vector<AttributeInfo>& GetAttrs() const
    {
	return vattrs;
    }
    virtual int GetValue(const char* attr, int* result) const
    {
        int r = 0;
	vr->Lock();
	if (vdlPlaybackGetEq(handler, &info) != 0)
	    r = -1;
	else if (info.cap & VEQ_CAP_BRIGHTNESS
		 && !strcasecmp(attr, VIDIX_BRIGHTNESS))
	    *result = info.brightness;
	else if (info.cap & VEQ_CAP_CONTRAST
		 && !strcasecmp(attr, VIDIX_CONTRAST))
	    *result = info.contrast;
	else if (info.cap & VEQ_CAP_SATURATION
		 && !strcasecmp(attr, VIDIX_SATURATION))
	    *result = info.saturation;
	else if (info.cap & VEQ_CAP_HUE
		 && !strcasecmp(attr, VIDIX_HUE))
	    *result = info.hue;
        else
	    r = -1;
        vr->Unlock();
	//printf("RESULT  %s  %d\n", attr, *result);
	return r;
    }
    virtual int SetValue(const char* attr, int value)
    {
        int r = 0;
        vr->Lock();
	if (!strcmp(attr, VIDIX_BRIGHTNESS))
	    info.brightness = value;
	else if (!strcmp(attr, VIDIX_CONTRAST))
	    info.contrast = value;
	else if (!strcmp(attr, VIDIX_SATURATION))
	    info.saturation = value;
	else if (!strcmp(attr, VIDIX_HUE))
	    info.hue = value;
	else
	    r = -1;

	if (r == 0 && vdlPlaybackSetEq(handler, &info) != 0)
	    r = -1;
        vr->Unlock();

	return r;
    }
};
#undef VIDIXCSTR

class VidixRenderer : public FullscreenRenderer
{
    VDL_HANDLE m_vidix_handler;
    vidix_capability_t m_vidix_cap;
    vidix_playback_t m_vidix_play;
    vidix_fourcc_t m_vidix_fourcc;
    vidix_yuv_t	dstrides;
    uint32_t apitch;
    uint8_t* vidix_mem;
    int next_frame;
    int image_Bpp;
    int x, y;
    uint32_t bgclr;
    VidixRtConfig* m_pVidixRtConfig;
    avm::vector<CImage*> ovlist;

public:
    VidixRenderer(PlayerWidget* pw, Display* _dpy,
		  int _width, int _height, fourcc_t fmt, bool _subtitles = false)
	:FullscreenRenderer(pw, _dpy, _width, _height, _subtitles),
        next_frame(0), bgclr(0), m_pVidixRtConfig(0)
    {
	if (vdlGetVersion() != VIDIX_VERSION)
	    throw FATAL("vidix incompatible library version");



	m_vidix_handler = vdlOpen(VIDIX_LIBDIR, 0, TYPE_OUTPUT, 0);
	//m_vidix_handler = vdlOpen(VIDIX_LIBDIR, "librage128.so", TYPE_OUTPUT, 0/*verbose*/);
	//m_vidix_handler = vdlOpen(VIDIX_LIBDIR, "libmga_crtc2.so", TYPE_OUTPUT, 0/*verbose*/);
	if (!m_vidix_handler)
	    throw FATAL("can't be opened");

	if (vdlGetCapability(m_vidix_handler, &m_vidix_cap) != 0)
	    throw FATAL("can't get capabilities");

	char fc[4];
	avm_set_le32(fc, fmt);
	AVM_WRITE("vidix", "Opened vidix renderer - FCC: %.4s\n", fc);
	AVM_WRITE("vidix", "Description: %s\n", m_vidix_cap.name);
	AVM_WRITE("vidix", "Author: %s\n", m_vidix_cap.author);

	if (((m_vidix_cap.maxwidth != -1) && (m_w > m_vidix_cap.maxwidth)) ||
	    ((m_vidix_cap.minwidth != -1) && (m_w < m_vidix_cap.minwidth)) ||
	    ((m_vidix_cap.maxheight != -1) && (m_h > m_vidix_cap.maxheight)) ||
	    ((m_vidix_cap.minwidth != -1 ) && (m_h < m_vidix_cap.minheight)))
	    throw FATAL("unsupported resolution (min/max width/height)");

	m_vidix_fourcc.fourcc = fmt;
        m_vidix_fourcc.srcw = m_w;
        m_vidix_fourcc.srch = m_h;
	if (vdlQueryFourcc(m_vidix_handler, &m_vidix_fourcc) != 0)
	    throw FATAL("unsupported fourcc");

	unsigned int err;
	switch (bit_depth)
	{
	case  1: err = VID_DEPTH_1BPP; break;
	case  2: err = VID_DEPTH_2BPP; break;
	case  4: err = VID_DEPTH_4BPP; break;
	case  8: err = VID_DEPTH_8BPP; break;
	case 12: err = VID_DEPTH_12BPP; break;
	case 15: err = VID_DEPTH_15BPP; break;
	case 16: err = VID_DEPTH_16BPP; break;
	case 24: err = VID_DEPTH_24BPP; break;
	case 32: err = VID_DEPTH_32BPP; break;
	default: err=~0U; break;
	}
	err = ((m_vidix_fourcc.depth & err) != err);
	if (m_vidix_fourcc.flags & VID_CAP_COLORKEY)
	{
	    vidix_grkey_t gr_key;
	    vdlGetGrKeys(m_vidix_handler, &gr_key);
	    gr_key.key_op = KEYS_PUT;
	    gr_key.ckey.op = CKEY_TRUE;
	    gr_key.ckey.red = 4;
	    gr_key.ckey.green = 0;
	    gr_key.ckey.blue = 247;
#if _SDL_VER > 1103
	    bgclr = SDL_MapRGBA(screen->format, gr_key.ckey.red,
				gr_key.ckey.green, gr_key.ckey.blue, 0);
#else
#warning RGB map missing
#endif
	    vdlSetGrKeys(m_vidix_handler, &gr_key);
	}
	doResize(pic_w, pic_h);

	delete m_pSdlgRtConfig;
        m_pSdlgRtConfig = 0;
        m_pVidixRtConfig = new VidixRtConfig(this, m_vidix_handler);
    }
    virtual ~VidixRenderer()
    {
        delete m_pVidixRtConfig;
	vdlPlaybackOff(m_vidix_handler);
	vdlClose(m_vidix_handler);
    }
    virtual IRtConfig* GetRtConfig() const { return m_pVidixRtConfig; }
    //virtual uint_t GetImages() const { return 0; }
    virtual CImage* ImageAlloc(const BITMAPINFOHEADER& bh, uint_t idx, uint_t align)
    {
	// vidix provides slow real graphics card memory
        // so it could be only usable with draw_band
	//printf("IMAGEALLOC %d  %d\n", idx, m_vidix_play.num_frames);
	Locker locker(m_Mutex);
	if (0 || idx >= m_vidix_play.num_frames || bh.biCompression != IMG_FMT_YV12)
            return 0;
	if (idx >= ovlist.size())
	{
	    BitmapInfo bi(m_w, m_h, m_vidix_fourcc.fourcc);
	    uint8_t* plane[3] =
	    {
		vidix_mem + m_vidix_play.offsets[idx]
		    + m_vidix_play.offset.y,
		    vidix_mem + m_vidix_play.offsets[idx]
		    + m_vidix_play.offset.u,
		    vidix_mem + m_vidix_play.offsets[idx]
		    + m_vidix_play.offset.v
	    };
	    int stride[3] = { dstrides.y, dstrides.u, dstrides.v };
	    //printf("stride %d %d %d\n", dstrides.y, dstrides.u, dstrides.v);
	    CImage* ci = new CImage(&bi, (const uint8_t**) plane, stride, false);
	    m_uiImages = ++idx;
	    ci->SetUserData((void*)idx);
	    ci->SetAllocator(this);
	    ovlist.push_back(ci);
	}
	return ovlist[idx];
    }
    virtual void ReleaseImages()
    {
	Locker locker(m_Mutex);
	m_uiImages = 0;
	if (image)
	    image->Release();
	image = 0;
	while (ovlist.size() > 0)
	{
            ovlist.back()->Release();
            ovlist.pop_back();
	}
    }
    virtual int Draw(const CImage* data)
    {
	// if we do not want to block our renderer while
	// keyboard event processing tries to lock this system
        // using just normal MUTEX - we are not writing anything to Xfree
        Locker locker(m_Mutex);
	if (!data)
	{
	    if (!image)
		return -1;
	    data = image;
	}
	m_lLastDrawStamp = longcount();

	data->AddRef();
	if (image)
	    image->Release();

	if (data->GetAllocator() == this)
	{
	    image = data;
            return 0;
	}
        //AVM_WRITE("DATA %x  %x\n", data->GetFmt()->biCompression, m_fmt);
	BitmapInfo bi(m_w, m_h, m_vidix_fourcc.fourcc);
	//printf("CONVERT  from  to ############  %.4s\n", (char*)&m_vidix_fourcc.fourcc);
	//data->GetFmt()->Print();
	//bi.Print();
	uint8_t* plane[3] =
	{
	    vidix_mem + m_vidix_play.offsets[next_frame]
		+ m_vidix_play.offset.y,
		vidix_mem + m_vidix_play.offsets[next_frame]
		+ m_vidix_play.offset.u,
		vidix_mem + m_vidix_play.offsets[next_frame]
		+ m_vidix_play.offset.v
	};
	int stride[3] = { dstrides.y, dstrides.u, dstrides.v };

	//printf("%.4s  %d %d %d  %p %p %p  %p %p %p\n", (char*)&m_vidix_fourcc.fourcc, stride[0], stride[1], stride[2],
	//plane[0], plane[1], plane[2], data->Data(0), data->Data(1), data->Data(2));
	// FIXME direct memcopy should be handled with DMA here

        // reading memory from MGA G400 is about 22 times slower then real RAM
	//int64_t ys = longcount(); int b = 0; for(int i = 0; i < 100000; i++) b += plane[0][i]; int64_t ye = longcount();
	//int64_t xs = longcount(); int a = 0; for(int i = 0; i < 100000; i++) a += data->m_pPlane[0][i]; int64_t xe = longcount();
        //printf("XF  %d  %lld    %d  %lld\n", a, xe - xs, b, ye - ys);
	CImage* ci = new CImage(&bi, (const uint8_t**) plane, stride, false);
	ci->Convert(data);
	data->Release();
	image = ci;

        return 0;
    }
    virtual int Sync()
    {
	if (m_vidix_play.num_frames > 1)
	{
	    Locker locker(m_Mutex);
	    if (image && image->GetAllocator() == this)
	    {
		next_frame = (int)image->GetUserData() % m_uiImages;
		//printf("NEXT  %d   %d\n", next_frame, m_vidix_play.num_frames);
	    }
	    vdlPlaybackFrameSelect(m_vidix_handler, next_frame);
	    next_frame = (next_frame + 1) % m_vidix_play.num_frames;
	}
	emutex.Lock();
	econd.Broadcast();
	emutex.Unlock();
	return 0;
    }
    virtual int Refresh()
    {
	Lock();
	int nx = 0, ny = 0;
	doGetPosition(nx, ny);
	//printf("XPO  %d  %d   %d   %d\n", nx, ny, x, y);
	if (x != nx || y != ny || screen->flags & SDL_FULLSCREEN)
	    doResize(pic_w, pic_h);
	Unlock();
	return FullscreenRenderer::Refresh();
    }
protected:
    virtual int doResize(int& new_w, int& new_h)
    {
	int i = FullscreenRenderer::doResize(new_w, new_h);
	//printf("DO RESIZE  %d\n", i);
	reinit();

	SDL_FillRect(screen, &cliprect, bgclr);
	SDL_UpdateRect(screen, cliprect.x, cliprect.x,
		       cliprect.w, cliprect.h);
        return i;
    }
    void reinit()
    {
        int err;

	//printf("FLAG %x %x\n", screen->flags & SDL_FULLSCREEN, screen->flags);
	if (screen->flags & SDL_FULLSCREEN)
	    x = y = 0;
	else
	    doGetPosition(x, y);
	//printf("POS X %d  Y %d  \n", x, y);

	memset(&m_vidix_play, 0, sizeof(vidix_playback_t));
	m_vidix_play.fourcc = m_vidix_fourcc.fourcc;
	m_vidix_play.capability = m_vidix_cap.flags; /* every ;) */
	m_vidix_play.blend_factor = 0; /* for now */

	m_vidix_play.src.x = (x < 0) ? 0 * -x : 0;
	m_vidix_play.src.y = (y < 0) ? 0 * -y : 0;
	m_vidix_play.src.w = (x < 0) ? m_w + 0*x : m_w;
	m_vidix_play.src.h = (y < 0) ? m_h + 0*y : m_h;

	m_vidix_play.dest.x = (x < 0) ? 0 : x;
	m_vidix_play.dest.y = (y < 0) ? 0 : y;
	m_vidix_play.dest.w = (x < 0) ? pic_w + x : pic_w;
	m_vidix_play.dest.h = (y < 0) ? pic_h + y : pic_h;
	m_vidix_play.num_frames = 6;//vo_doublebuffering?NUM_FRAMES-1:1;

	m_vidix_play.src.pitch.y =
	    m_vidix_play.src.pitch.u =
	    m_vidix_play.src.pitch.v = 0;

	vidix_video_eq_t vinfo;
	vdlPlaybackGetEq(m_vidix_handler, &vinfo);
	vdlPlaybackOff(m_vidix_handler);
	if (vdlConfigPlayback(m_vidix_handler, &m_vidix_play) != 0)
            throw FATAL("can't configure playback");

	vidix_mem = (uint8_t*) m_vidix_play.dga_addr;

	// vdlPlaybackFrameSelect(vidix_handler,next_frame);

	/* clear every frame with correct address and frame_size */
	for (unsigned i = 0; i < m_vidix_play.num_frames; i++)
	    memset(vidix_mem + m_vidix_play.offsets[i], 0x80,
		   m_vidix_play.frame_size);
	vdlPlaybackOn(m_vidix_handler);
	vdlPlaybackSetEq(m_vidix_handler, &vinfo);
	//if ((err = vdlPlaybackOn(m_vidix_handler))!=0)
	//    throw FATAL("can't start playback: %s\n", strerror(err));
	//printf("PITCH %d  %d  %d\n",m_vidix_play.dest.pitch.y,m_vidix_play.dest.pitch.u,m_vidix_play.dest.pitch.v);
	apitch = m_vidix_play.dest.pitch.y - 1;
	dstrides.y = (m_w + apitch) & ~apitch;
	dstrides.u = dstrides.v = 0;

	switch(m_vidix_fourcc.fourcc)
	{
	case IMG_FMT_YV12:
	case IMG_FMT_I420:
	case IMG_FMT_IYUV:
	case IMG_FMT_YVU9:
	case IMG_FMT_IF09:
	case IMG_FMT_Y800:
	case IMG_FMT_Y8:
	    dstrides.v = dstrides.u = dstrides.y;
	    image_Bpp=1;
	    break;
	case IMG_FMT_RGB32:
	case IMG_FMT_BGR32:
	    dstrides.y *= 4;
	    image_Bpp=4;
	    break;
	case IMG_FMT_RGB24:
	case IMG_FMT_BGR24:
	    dstrides.y *= 3;
	    image_Bpp=3;
	    break;
	default:
	    dstrides.y *= 2;
	    image_Bpp=2;
	    break;
	}

	switch(m_vidix_fourcc.fourcc)
	{
	    case IMG_FMT_YVU9:
	    case IMG_FMT_IF09:
		dstrides.u /= 4;
		dstrides.v /= 4;
		break;
	    case IMG_FMT_I420:
	    case IMG_FMT_IYUV:
	    case IMG_FMT_YV12:
		dstrides.u /= 2;
		dstrides.v /= 2;
		break;
	}

	//printf("strideB %d %d %d\n", dstrides.y, dstrides.u, dstrides.v);
/*	apitch = m_vidix_play.dest.pitch.y - 1;
	dstrides.y = (dstrides.y + apitch) & ~apitch;
	apitch = m_vidix_play.dest.pitch.v - 1;
	dstrides.v = (dstrides.v + apitch) & ~apitch;
	apitch = m_vidix_play.dest.pitch.u - 1;
	dstrides.u = (dstrides.u + apitch) & ~apitch;
  */
	//printf("strideC %d %d %d\n", dstrides.y, dstrides.u, dstrides.v);
	//printf("strideXX %d %d %d\n", m_vidix_play.dest.pitch.y, m_vidix_play.dest.pitch.v, m_vidix_play.dest.pitch.u);
    }
};
#endif /* HAVE_VIDIX */


IVideoRenderer* CreateFullscreenRenderer(IPlayerWidget* pw, void* dpy,
					int width, int height, bool sub)
{
    return new FullscreenRenderer(pw, (Display*)dpy, width, height, sub);
}

IVideoRenderer* CreateYUVRenderer(IPlayerWidget* pw, void* dpy,
				 int width, int height,
				 fourcc_t yuv_fmt, bool sub)
{
    //return new XvYUVRenderer(pw, dpy, width, height, yuv_fmt, sub);
#ifdef HAVE_VIDIX
    if (geteuid() == 0)
	// for root user try to use VIDIX
	try { return new VidixRenderer(pw, (Display*)dpy, width, height, yuv_fmt, sub);
	} catch (FatalError& e)  { e.PrintAll(); }
#endif
#if _SDL_VER > 1103
    return new YUVRenderer(pw, (Display*)dpy, width, height, yuv_fmt, sub);
#else
    AVM_WRITE("renderer", "SDL library too old - no XV support available\n");
    return 0;
#endif
}

IVideoRenderer* CreateVidixRenderer(IPlayerWidget* pw, void* dpy,
				   int width, int height,
				   fourcc_t yuv_fmt, bool sub)
{
#ifdef HAVE_VIDIX
    return new VidixRenderer(pw, (Display*)dpy, width, height, yuv_fmt, sub);
#else
    AVM_WRITE("renderer", "library compiled without VIDIX support - sorry no rendering\n");
    return 0;
#endif
}

#else /* HAVE_LIBSDL */

static const char* norend =
"\n\n!!! libaviplay library has been compiled WITHOUT SDL support !!!\n"
" sorry no rendering available - if you want to see VIDEO\n"
" recompile library with SDL support - check why detection of SDL failed\n"
" if you have problems send message to avifile@prak.org mailing list\n\n";

IVideoRenderer* CreateFullscreenRenderer(IPlayerWidget* pw, void* dpy,
					int width, int height, bool sub)
{
    AVM_WRITE("renderer", norend);
    return 0;
}

IVideoRenderer* CreateYUVRenderer(IPlayerWidget* pw, void* dpy,
				 int width, int height,
				 fourcc_t yuv_fmt, bool sub)
{
    AVM_WRITE("renderer", norend);
    return 0;
}

#endif /* HAVE_LIBSDL */

#undef __MODULE__

AVM_END_NAMESPACE;

#endif  /* X_DISPLAY_MISSING */
