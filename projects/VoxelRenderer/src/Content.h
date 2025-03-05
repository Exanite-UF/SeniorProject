#pragma once

#include <string>

class Content
{
public:
    constexpr static std::string_view assignMaterialComputeShader = "content/AssignMaterial.compute.glsl";
    constexpr static std::string_view blitFragmentShader = "content/Blit.fragment.glsl";
    constexpr static std::string_view blitFramebufferFragmentShader = "content/BlitFramebuffer.fragment.glsl";

    
    constexpr static std::string_view makeMipMapComputeShader = "content/MakeMipMap.compute.glsl";
    constexpr static std::string_view makeNoiseComputeShader = "content/MakeNoise.compute.glsl";
    constexpr static std::string_view screenTriVertexShader = "content/ScreenTri.vertex.glsl";
    
    constexpr static std::string_view renderReprojectionVertexShader = "content/ReprojectionRender.vertex.glsl";
    constexpr static std::string_view renderReprojectionFragmentShader = "content/ReprojectionRender.fragment.glsl";
    constexpr static std::string_view combineReprojectionFragmentShader = "content/CombineFrames.fragment.glsl";
    constexpr static std::string_view makeCombineMaskFragmentShader = "content/MakeCombineMask.fragment.glsl";

    constexpr static std::string_view defaultColorTexture = "content/DefaultColor.png";
    constexpr static std::string_view defaultNormalTexture = "content/DefaultNormal.png";


    constexpr static std::string_view resetHitInfoComputeShader = "content/PathTrace/ResetHitInfo.compute.glsl";
    constexpr static std::string_view resetVisualInfoComputeShader = "content/PathTrace/ResetVisualInfo.compute.glsl";
    constexpr static std::string_view prepareRayTraceFromCameraComputeShader = "content/PathTrace/PrepareRayTraceFromCamera.compute.glsl";
    constexpr static std::string_view fullCastComputeShader = "content/PathTrace/FullCast.compute.glsl";
    constexpr static std::string_view pathTraceToFramebufferShader = "content/PathTrace/ToFramebuffer.fragment.glsl";
};
