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

#ifndef _SDL_FONT_H_
#define _SDL_FONT_H_

#include <Resources/ITextureResource.h>
#include <Resources/IFontResource.h>
#include <Resources/IFontTextureResource.h>
#include <Resources/IResourcePlugin.h>
#include <Core/IListener.h>
#include <Math/Vector.h>
#include <string.h>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/weak_ptr.hpp>

//@todo make a SDL_ttf in Meta to suppot different platforms
#include <SDL_ttf.h>

namespace OpenEngine {
namespace Resources {

using Math::Vector;
using namespace std;

class SDLFont;
typedef boost::shared_ptr<SDLFont> SDLFontPtr;

/**
 * SDL Font resource.
 *
 * @class SDLFont SDLFont.h Resources/SDLFont.h
 */
class SDLFont : public IFontResource {
private:
    class SDLFontTexture : public IFontTextureResource, public Core::IListener<FontChangedEventArg> {
    private:
        SDLFontPtr font;
        int id;                     //!< texture identifier
        unsigned char* data;        //!< binary texture data
        unsigned int width;         //!< texture width
        unsigned int height;        //!< texture height
        unsigned int depth;         //!< texture depth/bits
        string text;                //!< font text
        boost::weak_ptr<SDLFontTexture> weak_this;
        void FireChangedEvent();
        friend class SDLFont;
    public:
        SDLFontTexture(SDLFontPtr font);
        virtual ~SDLFontTexture();
        
        void Handle(FontChangedEventArg arg);
        // texture resource methods
        void Load() {};
        void Unload() {};
        int GetID();
        void SetID(int id);   
        unsigned int GetWidth();
        unsigned int GetHeight();
        unsigned int GetDepth();
        unsigned char* GetData();
        ColorFormat GetColorFormat();
        //font texture resource methods
        void SetText(string text);
        string GetText();
    };

    typedef boost::shared_ptr<SDLFontTexture> SDLFontTexturePtr;
    TTF_Font* font;         //!< sdl font object
    string filename;        //!< file name
    int ptsize;             //!< font size (based on 72DPI)
    int style;
    Vector<3,float> colr;
    SDL_Color sdlcolr;
    boost::weak_ptr<SDLFont> weak_this;

    friend class SDLFontPlugin;

    void FireChangedEvent();
    SDLFont();
    SDLFont(string file);
public:
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & boost::serialization::base_object<IFontResource>(*this);
        ar & filename;
        ar & ptsize;
        ar & style;
        ar & colr;
        ar & sdlcolr;
    }
    
    ~SDLFont();
    
    // resource methods
    void Load();
    void Unload();

    // font resource methods
    IFontTextureResourcePtr CreateFontTexture();
    void Render(SDLFontTexture* tex);
    void SetPointSize(int ptsize);
    int GetPointSize();
    void SetFontStyle(int style);
    int GetFontStyle();
    void SetFontColor(Vector<3,float> colr);
    Vector<3,float> GetFontColor();

};

/**
 * SDL true type font resource plug-in.
 *
 * @class SDLFontPlugin SDLFont.h Resources/SDLFont.h
 */
class SDLFontPlugin : public IResourcePlugin<IFontResource> {
public:
    SDLFontPlugin();
    IFontResourcePtr CreateResource(string file);
};

} //NS Resources
} //NS OpenEngine

BOOST_CLASS_EXPORT(OpenEngine::Resources::SDLFont)

#endif // _SDL_FONT_H_
