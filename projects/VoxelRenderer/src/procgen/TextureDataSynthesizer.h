#pragma once

// Generate 2D or 3D textures (may need to be split)
// Many different types of noise all representable as textures (ex. Perlin, Gaussian)
// Can be used by grid synthesizer as pre-generated probability at each tile. Grid's size would be smaller or equal, but otherwise arbitrary
// Can be used by point synthesizer for choosing random points
// Can be representation of rasterized shape or equations, hence why not called noise synthesizer
class TextureDataSynthesizer {
    
};