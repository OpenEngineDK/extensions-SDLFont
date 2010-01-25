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
 * @param tex the SDLFontTexture that should be rendered.
 **/
void SDLFont::Render(SDLFontTexture* tex) {
   if (!font) throw Exception("SDLFont: Font not loaded.");
   SDL_PixelFormat format;
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

   SDL_Surface* dest = NULL;
   if (tex->fixed_size) {
       dest = SDL_CreateRGBSurfaceFrom(tex->data, 
                                       tex->width, 
                                       tex->height, 
                                       tex->depth, 
                                       tex->depth/8 * tex->width, 
                                       format.Rmask, 
                                       format.Gmask, 
                                       format.Bmask, 
                                       format.Amask);
       Vector<4,float> bgcolr = tex->bgcolr;
       Uint8 r = int(roundf(bgcolr[0] * 255));
       Uint8 g = int(roundf(bgcolr[1] * 255));
       Uint8 b = int(roundf(bgcolr[2] * 255));
       Uint8 a = int(roundf(bgcolr[3] * 255));
       Uint32 c = 0;//SDL_MapRGBA(&format, r, g, b, a);
       // weird hack (maybe sdl_maprgba does not work?)
       c |= r << 24;
       c |= g << 16;
       c |= b << 8;
       c |= a << 0;
       SDL_FillRect(dest, NULL, c);
   }       

   if (tex->text.empty()) {
       if (!tex->fixed_size) {
           if (tex->data) delete[] tex->data;
           // create texture of one pixel with all-zero values.       
           //@todo this is kind of hackish ... what to do?
           tex->data = new unsigned char[4];
           *((int*)tex->data) = 0;
           tex->width = 1;
           tex->height = 1;
           tex->depth = 32;
           tex->FireChangedEvent();
           return;
       }
       else {
           SDL_FreeSurface(dest);
           tex->FireChangedEvent();
           return; 
       }
   }

   TTF_SetFontStyle(font, style);
   SDL_Surface* surf = TTF_RenderText_Blended(font, tex->text.c_str(), sdlcolr);
   if (!surf) 
       throw ResourceException("SDLFont: Error rendering font");
   tex->depth = surf->format->BitsPerPixel;
   if (tex->depth != 32) 
       throw ResourceException("SDLFont: Unsupported font surface.");

   // convert to rgba colors 
   //@todo conversion not needed when color is white. This is often
   //the case when we render white text and use opengl to set color.
   SDL_Surface* converted = SDL_ConvertSurface(surf, &format, SDL_SWSURFACE);
   if (converted == NULL)
       throw ResourceException("SDLFont: Error converting SDL_ttf surface");
   
   if (tex->fixed_size) {
       // weird blending stuff ?????
       if (tex->bgcolr[3] == 0.0f) SDL_SetAlpha(converted, 0, 0);
       else SDL_SetAlpha(converted, SDL_SRCALPHA, 255);
       if (SDL_BlitSurface(converted, &converted->clip_rect, dest, &dest->clip_rect) != 0)
           throw ResourceException("SDLFont: Error blitting surface.");
       SDL_FreeSurface(dest);
   }
   else {
       tex->width  = surf->w;
       tex->height = surf->h;
       unsigned long size = (tex->depth/8) * tex->width * tex->height;
       tex->data = new unsigned char[size];
       SDL_LockSurface(converted);
       memcpy(tex->data, converted->pixels, size);
       SDL_UnlockSurface(converted);
   }
   SDL_FreeSurface(converted);
   SDL_FreeSurface(surf);
   tex->FireChangedEvent();
}

/**
 * Create a new SDLFontTexture of dynamic size. The texture will be bound to this
 * SDLFont and will be re-rendered by the SDLFont each time either the
 * text of the FontTexture changes or the SDLFont is updated.
 * 
 * @return a smart pointer to the new SDLFontTexture.
 **/
IFontTextureResourcePtr SDLFont::CreateFontTexture() {
    if (!font) 
        throw Exception("SDLFont: Font not loaded");
    SDLFontTexture* tex = new SDLFontTexture(SDLFontPtr(weak_this));
    SDLFontTexturePtr ptr(tex);
    tex->weak_this = ptr;
    Render(tex);
    return ptr;
}

/**
 * Create a new SDLFontTexture of fixed size. The texture will be bound to this
 * SDLFont and will be re-rendered by the SDLFont each time either the
 * text of the FontTexture changes or the SDLFont is updated.
 * 
 * @return a smart pointer to the new SDLFontTexture.
 **/
IFontTextureResourcePtr SDLFont::CreateFontTexture(int fixed_width, int fixed_height) {
    if (!font) 
        throw Exception("SDLFont: Font not loaded");
    SDLFontTexture* tex = new SDLFontTexture(SDLFontPtr(weak_this), fixed_width, fixed_height);
    SDLFontTexturePtr ptr(tex);
    tex->weak_this = ptr;
    Render(tex);
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
SDLFont::SDLFontTexture::SDLFontTexture(SDLFontPtr font)
    : font(font)
    , id(0)
    , data(NULL) 
    , fixed_size(false)
{
    font->ChangedEvent().Attach(*this);
}

SDLFont::SDLFontTexture::SDLFontTexture(SDLFontPtr font, int fixed_width, int fixed_height)
    : font(font)
    , id(0)
    , data(NULL) 
    , width(fixed_width)
    , height(fixed_height)
    , depth(32)
    , fixed_size(true)
{
    font->ChangedEvent().Attach(*this);
    data = new unsigned char[depth/8 * width * height];
}

SDLFont::SDLFontTexture::~SDLFontTexture() {
    if (data) { 
        delete[] data;
        data = NULL;
    }
    font->ChangedEvent().Detach(*this);
}

void SDLFont::SDLFontTexture::Handle(FontChangedEventArg arg) {
    font->Render(this);
}

int SDLFont::SDLFontTexture::GetID() {
    return id;
}

void SDLFont::SDLFontTexture::SetID(int id) {
    this->id = id;
}	

unsigned int SDLFont::SDLFontTexture::GetWidth() {
    return width;
}

unsigned int SDLFont::SDLFontTexture::GetHeight() {
    return height;
}

unsigned int SDLFont::SDLFontTexture::GetDepth() {
    return depth;
}

unsigned char* SDLFont::SDLFontTexture::GetData() {
    return data;
}

ColorFormat SDLFont::SDLFontTexture::GetColorFormat() {
    if (depth == 32)
        return RGBA;
    else if (depth == 24)
        return RGB;
    else if (depth == 8)
        return LUMINANCE;
    else
        throw Exception("unknown color depth");
}

void SDLFont::SDLFontTexture::SetText(string text) {
    this->text = text;
    font->Render(this);
}

string SDLFont::SDLFontTexture::GetText() {
    return text;
}

void SDLFont::SDLFontTexture::SetBackground(Vector<4,float> color) {
    bgcolr = color;
    FireChangedEvent();
}

Vector<4,float> SDLFont::SDLFontTexture::GetBackground() {
    return bgcolr;
}

void SDLFont::SDLFontTexture::FireChangedEvent() {
    changedEvent.
        Notify(TextureChangedEventArg(this));
}

} //NS Resources
} //NS OpenEngine
