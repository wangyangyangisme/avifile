
#include <avm_map.h>
#include <avm_output.h>
#include <image.h>
#include <string.h>

#ifdef HAVE_LIBFREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H

//#include <freetype/freetype.h>

#if (FREETYPE_MAJOR > 2) || (FREETYPE_MAJOR == 2 && FREETYPE_MINOR >= 1)
#define HAVE_FREETYPE21
#endif

AVM_BEGIN_NAMESPACE;

struct ci_font_t
{
    ci_surface_t* bitmap;
    int width;
    int height;
    int index; // local vars - used by ft2
};

static inline int int_f266(int x) { return x * 64; }
static inline int f266_int(int x) { return x / 64; }

class FontRend_FT {
public:
    FontRend_FT() : m_Lib(0), m_Face(0) {}

    int Open(const char* filename, int faceindex, int size)
    {
	if (FT_Init_FreeType(&m_Lib))
	{
	    AVM_WRITE("FTREND", "error Open FT Library\n");
            return -1;
	}

        int r = FT_New_Face(m_Lib, filename, 0, &m_Face);
	if (r == FT_Err_Unknown_File_Format)
	{
	    AVM_WRITE("FTREND", "the font file could be opened and read,"
		      "but it appears that its font format is unsupported\n");
	    return -1;
	}
	else if (r)
	{
	    AVM_WRITE("FTREND", "font file can't be open\n");
	    return -1;
	}

	FT_Select_Charmap(m_Face, ft_encoding_unicode);
	if (!m_Face->charmap || m_Face->charmap->encoding != ft_encoding_unicode)
	{
	    AVM_WRITE("FTREND", "unicode charmap not available for this font!\n");
	    if (FT_Set_Charmap(m_Face, m_Face->charmaps[0]))
		AVM_WRITE("FTREND", "no charmaps!\n");
	}

	if (FT_IS_SCALABLE(m_Face))
	{
	    if (FT_Set_Char_Size(m_Face, 0, int_f266(size), 0, 0)); //72dpi
		AVM_WRITE("FTREND", "Set_Char_Size for scalable font fails???");
	}
	else
	{
            // FIXME Set_Pixel_Sizes

	}

	if (FT_IS_FIXED_WIDTH(m_Face))
	    AVM_WRITE("FTREND", "fixed width.\n");

        m_bKerning = FT_HAS_KERNING(m_Face);
	// read font
        return 0;
    }
    void Close()
    {
        FT_Done_FreeType(m_Lib);
    }
    const ci_font_t* GetChar(int ch, const ci_font_t* prev = 0, int* kern_x = 0)
    {
	const ci_font_t* t = m_Map.find(ch);
	if (!t)
	{
	    ci_font_t n;

	    n.index = FT_Get_Char_Index(m_Face, ch);
	    int r = FT_Load_Glyph(m_Face, n.index,
				  FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING);

	    FT_Bitmap bit;
	    if (m_Face->glyph->format == ft_glyph_format_outline)
	    {
                //
                bit.pixel_mode = ft_pixel_mode_mono;
		//bit.rows =

		bit.pixel_mode = ft_pixel_mode_grays;
                memset(bit.buffer, 0, bit.rows * bit.pitch);
	    }
	    //if (FT_Get_Glyph_Bitmap(m_Face, &bit))
	    //    AVM_WRITE("FTREND", "error GetGlyphBitmap\n");
	    m_Map.insert(ch, n);
            t = m_Map.find(ch);
	}

	if (m_bKerning && prev && kern_x)
	{
	    FT_Vector delta;
	    FT_Get_Kerning(m_Face, prev->index, t->index,
			   ft_kerning_default, &delta);
            *kern_x = f266_int(delta.x);
	}
        return t;
    }
protected:
    avm::avm_map<int, ci_font_t> m_Map;
    FT_Library m_Lib;
    FT_Face m_Face;
    int m_iSpace;

    bool m_bKerning;
};

AVM_END_NAMESPACE;

#endif // HAVE_FREETYPE
