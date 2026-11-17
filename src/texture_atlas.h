#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include <cassert>
#include <iostream>

#include "vector_util.h"
#include <SFML/Graphics.hpp>

// RectPacker maintains two offsets into an overall bin size:
//  - A line y-offset, representing the current horizontal line on which to place new rectangles
//      horizontally along (the line corresponding to the rectangle tops)
//  - An x-offset into the line, representing the x-coordinate to align the next rectangle left-side
// Furthermore it maintains the maximum texture height encountered along the current line.
// The RectPacker places rectangles left to right along this line until it cannot fit the next
//  rectangle. Once this happens, the line is shifted down to meet the bottom of the tallest texture
//  placed on the line, and the x-offset is reset. This continues until the next rectangle
//  packed cannot be fit on the line within the bin space.
class RectPacker {
public:
    RectPacker(const sf::Vector2f& size) 
    : offset(0.0f), rectPlacement(), tallest(0.0f), binSize(size), lineY(0.0f) {};

    std::pair<bool, size_t> pack(const sf::Vector2f& rectSize) { 
        if (offset + rectSize.x > binSize.x){
            lineY += tallest;
            offset = 0.0f;
            tallest = 0.0f;
        } 

        if(lineY + rectSize.y > binSize.y){
            return {false, std::numeric_limits<size_t>::max() };
        }

        sf::FloatRect rectangle{{offset, lineY}, rectSize};
        size_t id = rectPlacement.size();
        rectPlacement.push_back(rectangle);
        offset += rectSize.x;
        if(tallest < rectSize.y){
            tallest = rectSize.y;
        }

        return {true, id};
        

    }

    // Returns the coordinates and size of a previously returned rectangle id.
    // Note that in this coordinate system, y points downwards from the origin at the top-left.
    sf::FloatRect getBounds(size_t rectID) const { return  rectPlacement[rectID];}

private:
    float offset;
    std::vector<sf::FloatRect> rectPlacement;
    float tallest;
    sf::Vector2f binSize;
    float lineY;
};

// Returns a new image which pages the given img with *pixels* pixels on each side.
// The colour of new border pixels are taken by sampling the closest valid pixel in the original
// image.
//
// NOTE: a production implementation might also support other edge wrapping behaviors, like cycling
// around to the other side of the texture. See https://learnopengl.com/Getting-started/Textures
// Our implementation corresponds to GL_CLAMP_TO_EDGE
inline sf::Image padImage(const sf::Image& img, unsigned int pixels) { 
    unsigned int width = img.getSize().x;
    unsigned int height = img.getSize().y;
    sf::Image result({width + 2 * pixels, height + 2 * pixels}, sf::Color::Transparent);

    for(unsigned int y=0; y < height + 2 * pixels; y++){
        for(unsigned int x=0; x < width + 2 * pixels; x++){
            int sourceX = (int)x - (int)pixels;
            if(sourceX < 0){
                sourceX = 0;
            }   
            if(sourceX > (int)width - 1){
                sourceX = width - 1;
            }

            int sourceY = (int)y - (int)pixels;
            if(sourceY < 0){
                sourceY = 0;
            }
            if(sourceY > (int)height - 1){
                sourceY = height - 1;
            }
            sf::Color c = img.getPixel({(unsigned int)sourceX, (unsigned int)sourceY});
            result.setPixel({x, y}, c);

        }
    }

    return result;
 };

using TextureID = size_t;

class TextureAtlas {
public:
    TextureAtlas(sf::Vector2u binSize, size_t padding)
        : mTexture(binSize), mPacker(sf::Vector2f(mTexture.getSize())), mPadding{padding} {}

    TextureID registerTexture(const std::string& filepath) { 
        sf::Image img;
        bool loaded = img.loadFromFile(filepath);
        assert(loaded);

        sf::Image padded = padImage(img, mPadding);
        
        auto [ok, id] = mPacker.pack(sf::Vector2f(padded.getSize()));
        assert(ok);

        sf::FloatRect rect = mPacker.getBounds(id);
        mTexture.update(padded, sf::Vector2u(rect.position));

        return id;
     }

    sf::FloatRect getBoundsUV(TextureID id) const { 
        sf::FloatRect rect = mPacker.getBounds(id);
        sf::Vector2f atlas(mTexture.getSize());

        float left = rect.position.x + mPadding;
        float top = rect.position.y + mPadding;
        float width = rect.size.x - 2 * mPadding;
        float height = rect.size.y - 2 * mPadding;

        float u = left / atlas.x;
        float v = 1.0f - (top + height) / atlas.y;
        return sf::FloatRect({u,v}, {width / atlas.x, height / atlas.y});
    }

    sf::Vector2u getSizePixels(TextureID id) const { 
        sf::FloatRect rect = mPacker.getBounds(id);
        return sf::Vector2u((unsigned int)(rect.size.x - 2 * mPadding), (unsigned int)(rect.size.y - 2 * mPadding));
     }

    const sf::Texture& getTexture() const { return mTexture; }

    

private:
    sf::Texture mTexture;
    RectPacker mPacker;
    size_t mPadding;
};

#endif  // TEXTURE_ATLAS_H
