#pragma once

#include <cstdint>
#include <src/procgen/TextureData.h>

// Generate 2D or 3D textures (may need to be split)
// Many different types of noise all representable as textures (ex. Perlin, Gaussian)
// Can be used by grid synthesizer as pre-generated probability at each tile. Grid's size would be smaller or equal, but otherwise arbitrary
// Can be used by point synthesizer for choosing random points
// Can be representation of rasterized shape or equations, hence why not called noise synthesizer
class TextureDataSynthesizer 
{
    float seed;

public:
    virtual void generate(TextureData& textureData) = 0;
    virtual void showDebugMenu() = 0;
};