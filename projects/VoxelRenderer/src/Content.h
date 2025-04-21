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
    constexpr static std::string_view afterCastComputerShader = "content/PathTrace/AfterCast.compute.glsl";

    constexpr static std::string_view resetPrimaryRayInfoComputeShader = "content/PathTrace/ResetPrimaryRayInfo.compute.glsl";
    constexpr static std::string_view beforeCastComputeShader = "content/PathTrace/BeforeCast.compute.glsl";
    constexpr static std::string_view castPrimaryRayComputeShader = "content/PathTrace/PrimaryRay.compute.glsl";
    constexpr static std::string_view groupPixelsComputeShader = "content/PathTrace/GroupPixels.compute.glsl";

    // SVGF
    constexpr static std::string_view integrateFrameFragmentShader = "content/SVGF/IntegrateFrame.fragment.glsl";
    constexpr static std::string_view firstWaveletIterationFragmentShader = "content/SVGF/WaveletIterationFirst.fragment.glsl";
    constexpr static std::string_view waveletIterationFragmentShader = "content/SVGF/WaveletIteration.fragment.glsl";
    constexpr static std::string_view toFramebufferSVGFFragmentShader = "content/SVGF/ToFramebuffer.fragment.glsl";

    // Reprojection
    constexpr static std::string_view renderReprojectionVertexShader = "content/Reprojection/ReprojectionRender.vertex.glsl";
    constexpr static std::string_view renderReprojectionFragmentShader = "content/Reprojection/ReprojectionRender.fragment.glsl";
    constexpr static std::string_view bypassReprojectionFragmentShader = "content/Reprojection/Bypass.fragment.glsl";

    // Voxel Manipulation
    constexpr static std::string_view makeMipMapComputeShader = "content/VoxelManipulation/MakeMipMap.compute.glsl";
    constexpr static std::string_view makeNoiseComputeShader = "content/VoxelManipulation/MakeNoise.compute.glsl";
    constexpr static std::string_view assignMaterialComputeShader = "content/VoxelManipulation/AssignMaterial.compute.glsl";

    // Misc
    constexpr static std::string_view drawTextureFragmentShader = "content/DrawTexture.fragment.glsl";

    // Post Processing
    constexpr static std::string_view applyKernelLineFragmentShader = "content/PostProcessing/ApplyKernelLine.fragment.glsl";
    constexpr static std::string_view showOtherFragmentShader = "content/PostProcessing/ShowOther.fragment.glsl";

    constexpr static std::string_view denoiseShader = "content/PostProcessing/Denoise.fragment.glsl";
    constexpr static std::string_view denoise2Shader = "content/PostProcessing/Denoise2.fragment.glsl";

    constexpr static std::string_view toneMapShader = "content/PostProcessing/ToneMap.fragment.glsl";
    constexpr static std::string_view showAngularSizeShader = "content/PostProcessing/ShowAngularSize.fragment.glsl";

    // Font
    constexpr static std::string_view imguiFont = "content/fonts/RobotoMono-Medium.ttf";

    class Triangulation
    {
    public:
        constexpr static std::string_view vertShaderPathTriangle = "content/Triangulation/Phong.vertex.glsl";
        constexpr static std::string_view fragShaderPathTriangle = "content/Triangulation/Phong.fragment.glsl";
        constexpr static std::string_view vertShaderPathVoxel = "content/Triangulation/Voxel.vertex.glsl";
        constexpr static std::string_view fragShaderPathVoxel = "content/Triangulation/Voxel.fragment.glsl";
        constexpr static std::string_view vertShaderPathRasterization = "content/Triangulation/ConservativeRasterization.vertex.glsl";
        constexpr static std::string_view fragShaderPathRasterization = "content/Triangulation/ConservativeRasterization.fragment.glsl";
        constexpr static std::string_view compShaderPathRayMarch = "content/Triangulation/RayMarching.comp.glsl";
    };
};
