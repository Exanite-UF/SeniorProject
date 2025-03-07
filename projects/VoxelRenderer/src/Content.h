#pragma once

#include <string>

class Content
{
public:
    constexpr static std::string_view blitFragmentShader = "content/Blit.fragment.glsl";
    constexpr static std::string_view blitFramebufferFragmentShader = "content/BlitFramebuffer.fragment.glsl";

    constexpr static std::string_view screenTriVertexShader = "content/ScreenTri.vertex.glsl";

    constexpr static std::string_view defaultColorTexture = "content/DefaultColor.png";
    constexpr static std::string_view defaultNormalTexture = "content/DefaultNormal.png";

    // Path tracing
    constexpr static std::string_view resetHitInfoComputeShader = "content/PathTrace/ResetHitInfo.compute.glsl";
    constexpr static std::string_view resetVisualInfoComputeShader = "content/PathTrace/ResetVisualInfo.compute.glsl";
    constexpr static std::string_view prepareRayTraceFromCameraComputeShader = "content/PathTrace/PrepareRayTraceFromCamera.compute.glsl";
    constexpr static std::string_view fullCastComputeShader = "content/PathTrace/FullCast.compute.glsl";
    constexpr static std::string_view pathTraceToFramebufferShader = "content/PathTrace/ToFramebuffer.fragment.glsl";

    // Reprojection
    constexpr static std::string_view renderReprojectionVertexShader = "content/Reprojection/ReprojectionRender.vertex.glsl";
    constexpr static std::string_view renderReprojectionFragmentShader = "content/Reprojection/ReprojectionRender.fragment.glsl";
    constexpr static std::string_view combineReprojectionFragmentShader = "content/Reprojection/CombineFrames.fragment.glsl";
    constexpr static std::string_view makeCombineMaskFragmentShader = "content/Reprojection/MakeCombineMask.fragment.glsl";

    // Voxel Manipulation
    constexpr static std::string_view makeMipMapComputeShader = "content/VoxelManipulation/MakeMipMap.compute.glsl";
    constexpr static std::string_view makeNoiseComputeShader = "content/VoxelManipulation/MakeNoise.compute.glsl";
    constexpr static std::string_view assignMaterialComputeShader = "content/VoxelManipulation/AssignMaterial.compute.glsl";

    // Misc
    constexpr static std::string_view drawTextureFragmentShader = "content/DrawTexture.fragment.glsl";

    // Post Processing
    constexpr static std::string_view applyKernelLineFragmentShader = "content/PostProcessing/ApplyKernelLine.fragment.glsl";

    constexpr static std::string_view denoiseShader = "content/PostProcessing/Denoise.fragment.glsl";
    constexpr static std::string_view denoise2Shader = "content/PostProcessing/Denoise2.fragment.glsl";
};
