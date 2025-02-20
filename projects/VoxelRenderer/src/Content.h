#pragma once

#include <string>

class Content
{
public:
    constexpr static std::string_view assignMaterialComputeShader = "content/AssignMaterial.compute.glsl";
    constexpr static std::string_view displayToWindowFragmentShader = "content/DisplayToWindow.fragment.glsl";
    constexpr static std::string_view executeRayTraceComputeShader = "content/ExecuteRayTrace.compute.glsl";
    constexpr static std::string_view makeMipMapComputeShader = "content/MakeMipMap.compute.glsl";
    constexpr static std::string_view makeNoiseComputeShader = "content/MakeNoise.compute.glsl";
    constexpr static std::string_view prepareRayTraceFromCameraComputeShader = "content/PrepareRayTraceFromCamera.compute.glsl";
    constexpr static std::string_view raymarcherFragmentShader = "content/Raymarcher.fragment.glsl";
    constexpr static std::string_view resetHitInfoComputeShader = "content/ResetHitInfo.compute.glsl";
    constexpr static std::string_view screenTriVertexShader = "content/ScreenTri.vertex.glsl";

    constexpr static std::string_view defaultColorTexture = "content/DefaultColor.png";
    constexpr static std::string_view defaultNormalTexture = "content/DefaultNormal.png";
};
