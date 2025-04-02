#pragma once

enum TextureType
{
    // Represents an unknown or invalid texture type
    Unknown = 0,

    // The texture will use the RGB8 format and be loaded using the sRGB colorspace
    // The texture will be converted to 0-1 when sampling, with colorspace conversion
    ColorOnly,

    // The texture will use the RGBA8 format and be loaded using the sRGB colorspace
    // The texture will be converted to 0-1 when sampling, with colorspace conversion
    ColorAlpha,

    // The texture will use the RG8 format and be loaded using the linear colorspace
    // The texture will be converted to 0-1 when sampling, but without colorspace conversion
    // The 3rd channel can be reconstructed when sampling
    Normal,
};
