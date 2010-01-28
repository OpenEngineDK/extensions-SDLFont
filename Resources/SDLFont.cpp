// SDL Font resource.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This extension is based on the sdlimage extension and uses sdl_ttf
// to handle font loading and rendering. 
//
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Resources/SDLFont.h>
#include <Resources/Exceptions.h>
#include <Resources/IFontTextureResource.h>
#include <Resources/ITextureResource.h>

#include <Logging/Logger.h>

namespace OpenEngine {
namespace Resources {

SDLFontPlugin::SDLFontPlugin() {
    this->AddExtension("ttf");
}

IFontResourcePtr SDLFontPlugin::CreateResource(string file) {
    // store a weak reference from SDLFont to itself.  this makes us
    // able give a shared pointer (reference counted) to new
    // SDLFontTextures upon construction.
   
    // @todo we need to be sure that this does not lead to a circular
    // reference and memory leaks.
    SDLFontPtr ptr(new SDLFont(file));
    ptr->weak_this = ptr;
    return ptr;
}

/**
 * Empty Constructor - only for boost serialization
 * 
 **/
SDLFont::SDLFont()
  : font(NULL)
  , ptsize(12)
  , style(FONT_STYLE_NORMAL)
  , colr(Vector<3,float>(1.0)) 
{
    Init();
}

/**
 * Construct a SDLFont from a ttf file.  
 *
 * Only create instances using the SDLFontPlugin to ensure proper
 * reference counting.
 *
 * @param filename a string containing a path to the ttf file.
 **/
SDLFont::SDLFont(string filename)
    : font(NULL)
    , filename(filename)
    , ptsize(12)
    , style(FONT_STYLE_NORMAL)
    , colr(Vector<3,float>(1.0))
{
    Init();
}

void SDLFont::Init() {
    format.palette = 0; format.colorkey = 0; format.alpha = 0;
    format.BitsPerPixel = 32; format.BytesPerPixel = 4;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    format.Rmask = 0xFF000000; format.Rshift = 0; format.Rloss = 0;
    format.Gmask = 0x00FF0000; format.Gshift = 8; format.Gloss = 0;
    format.Bmask = 0x0000FF00; format.Bshift = 16; format.Bloss = 0;
    format.Amask = 0x000000FF; format.Ashift = 24; format.Aloss = 0;
#else                                                               
    format.Rmask = 0x000000FF; format.Rshift = 24; format.Rloss = 0;
    format.Gmask = 0x0000FF00; format.Gshift = 16; format.Gloss = 0;
    format.Bmask = 0x00FF0000; format.Bshift = 8; format.Bloss = 0;
    format.Amask = 0xFF000000; format.Ashift = 0; format.Aloss = 0;
#endif

    sdlcolr.r = 255;
    sdlcolr.g = 255;
    sdlcolr.b = 255;
    if (!TTF_WasInit()) {
        if(TTF_Init() == -1) {
            throw ResourceException("Error initialising SDL_ttf. Description: " + string(TTF_GetError()));
        }
    }
}

/**
 * Destructor - calls Unload to free the SDL_ttf resources.
 * 
 **/
SDLFont::~SDLFont() {
    Unload();
}

/**
 * Helper function to notify listeners of a changed event.
 * 
 **/
void SDLFont::FireChangedEvent() {
    changedEvent.
        Notify(FontChangedEventArg(IFontResourcePtr(weak_this)));
}

/**
 * Load the SDL_ttf resources. 
 *
 * You should specify the font point size
 * before loading the font as SDL_ttf requires this upon
 * initialization.
 *
 * Call this function before any calls to CreateFontTexture. 
 * 
 **/
void SDLFont::Load() {
    if (font) 
        return;
    font = TTF_OpenFont(filename.c_str(), ptsize);
    if (!font)
        throw ResourceException("Error loading SDLFont data in: " + filename +". Description: " + string(TTF_GetError()));
}

/**
 * Unload the SDL_ttf font resources.
 * 
 * If the font is not loaded when an associated SDLFontTexture needs
 * rendering an exception will be raised.
 *
 **/
void SDLFont::Unload() {
    if (font) {
        TTF_CloseFont(font);
        font = NULL;
    }
    //@todo when to free sdl_ttf?
}

/**
 * Render an SDLFontTexture using this font.
 * 
 **/
void SDLFont::RenderText(string s, IFontTextureResourcePtr texr, int x, int y) {
    SDLFontTexture* tex = dynamic_cast<SDLFontTexture*>(texr.get());
    if (!tex) throw Exception("Font Texture not compatible with SDLFontResource.");
    if (s.empty()) return;
    if (!font) throw Exception("SDLFont: Font not loaded.");

    SDL_Surface* dest = tex->surface;
    TTF_SetFontStyle(font, style);
    SDL_Surface* surf = TTF_RenderText_Blended(font, s.c_str(), sdlcolr);
    if (!surf) 
        throw ResourceException("SDLFont: Error rendering font");

    // convert to rgba colors 
    //@todo conversion not needed when color is white. This is often
    //the case when we render white text and use opengl to set color.
    SDL_Rect rect = dest->clip_rect;
    rect.x = x;
    rect.y = y;
    rect.w -= x;
    rect.h -= y;
    SDL_Surface* converted = SDL_ConvertSurface(surf, &format, SDL_SWSURFACE);
    if (converted == NULL)
        throw ResourceException("SDLFont: Error converting SDL_ttf surface");
   
   // if (bgcolr[3] == 0.0f) SDL_SetAlpha(converted, 0, 0);
   // else SDL_SetAlpha(converted, SDL_SRCALPHA, 255);
   if (SDL_BlitSurface(converted, &converted->clip_rect, dest, &rect) != 0)
       throw ResourceException("SDLFont: Error blitting surface.");
   SDL_FreeSurface(converted);
   SDL_FreeSurface(surf);
   tex->FireChangedEvent(rect.x, rect.y, rect.w, rect.h);
}

Vector<2,int> SDLFont::TextDim(string s) {
    Vector<2,int> dim;
    TTF_SizeText(font, s.c_str(), &dim[0], &dim[1]);
    return dim;
}


/**
 * Create a new SDLFontTexture of fixed size. The texture will be bound to this
 * SDLFont and will be re-rendered by the SDLFont each time either the
 * text of the FontTexture changes or the SDLFont is updated.
 * 
 * @return a smart pointer to the new SDLFontTexture.
 **/
IFontTextureResourcePtr SDLFont::CreateFontTexture(int width, int height) {
    if (!font) 
        throw Exception("SDLFont: Font not loaded");
    SDLFontTexture* tex = new SDLFontTexture(format, width, height);
    SDLFontTexturePtr ptr(tex);
    tex->weak_this = ptr;
    return ptr;
}

/**
 * Set the size of the SDLFont. This will reload the SDLFont as this
 * is required by SDL_ttf.
 * 
 * @param ptsize the point size of the SDLFont.
 **/
void SDLFont::SetSize(int ptsize) {
    this->ptsize = ptsize;
    if (font) {
        Unload();
        Load();
    }
}
    
/**
 * Get the current size of the SDLFont. 
 * 
 * @return the point size of the SDLFont.
 **/
int SDLFont::GetSize() {
    return ptsize;
}

/**
 * Set the style of the SDLFont.  This could be one of
 * FONT_STYLE_NORMAL, FONT_STYLE_BOLD, FONT_STYLE_ITALIC,
 * FONT_STYLE_UNDERLINE, or any bitwise OR combination of the above.
 * 
 * @param style the style of the SDLFont
 **/
void SDLFont::SetStyle(int style) {
    this->style = style;
    FireChangedEvent();
}
    
/**
 * Get the current style of the SDLFont. 
 * 
 * @return the style of the SDLFont.
 **/
int SDLFont::GetStyle() {
    return style;
}

/**
 * Set the color of the SDLFont. Initially this color is set to white
 * which will allow the color to be determined by the applied texture
 * color when rendering the texture in a scene.
 * 
 * @param colr the color of the SDLFont
 **/
void SDLFont::SetColor(Vector<3,float> colr) {
    this->colr = colr;
    // calculate integer rgb values
    sdlcolr.r = int(roundf(colr[0] * 255));
    sdlcolr.g = int(roundf(colr[1] * 255));
    sdlcolr.b = int(roundf(colr[2] * 255));
    FireChangedEvent();
}

/**
 * Get the current color of the SDLFont. 
 * 
 * @return the color of the SDLFont.
 **/
Vector<3,float> SDLFont::GetColor() {
    return colr;
}

// font texture implementation
SDLFont::SDLFontTexture::SDLFontTexture(SDL_PixelFormat format,
                                        int width, int height)
    : IFontTextureResource()
    , form(format)
{
    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 
                                   format.Rmask, format.Gmask, 
                                   format.Bmask, format.Amask);
    this->width = width;
    this->height = height;
    data = (unsigned char*)surface->pixels;
    channels = 4;
    this->format = RGBA;
}

SDLFont::SDLFontTexture::~SDLFontTexture() {
    if (data) { 
        SDL_FreeSurface(surface);
        data = NULL;
    }
}

void SDLFont::SDLFontTexture::Clear(Vector<4,float> color) {
    Uint8 r = int(roundf(color[0] * 255));
    Uint8 g = int(roundf(color[1] * 255));
    Uint8 b = int(roundf(color[2] * 255));
    Uint8 a = int(roundf(color[3] * 255));
    Uint32 c = SDL_MapRGBA(&form, r, g, b, a);
    SDL_FillRect(surface, NULL, c);    
}

// void SDLFont::SDLFontTexture::SetText(string text) {
//     this->text = text;
//     font->Render(this);
// }

// string SDLFont::SDLFontTexture::GetText() {
//     return text;
// }

// void SDLFont::SDLFontTexture::SetBackground(Vector<4,float> color) {
//     bgcolr = color;
//     FireChangedEvent();
// }

// Vector<4,float> SDLFont::SDLFontTexture::GetBackground() {
//     return bgcolr;
// }

void SDLFont::SDLFontTexture::FireChangedEvent(int x, int y, int w, int h) {
    changedEvent.
        Notify(TextureChangedEventArg(ITextureResourcePtr(weak_this), x, y, w, h));
}

} //NS Resources
} //NS OpenEngine
