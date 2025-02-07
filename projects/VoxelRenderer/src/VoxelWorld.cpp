#include "VoxelWorld.h"

#include "GraphicsUtils.h"

VoxelWorld::VoxelWorld(GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram)
{
    this->makeNoiseComputeProgram = makeNoiseComputeProgram;
    this->makeMipMapComputeProgram = makeMipMapComputeProgram;
    this->assignMaterialComputeProgram = assignMaterialComputeProgram;

    this->currentNoiseTime = 0;

    // Make and fill the buffers
    uint16_t width = 512;
    uint16_t height = 512;
    uint16_t depth = 512;

    this->occupancyMap = GraphicsUtils::create3DImage(width / 2, height / 2, depth / 2, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
    this->mipMap1 = GraphicsUtils::create3DImage(width / 8, height / 8, depth / 8, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
    this->mipMap2 = GraphicsUtils::create3DImage(width / 32, height / 32, depth / 32, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
    // this->mipMap3 = GraphicsUtils::create3DImage(8, 8, 8, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
    // this->mipMap4 = GraphicsUtils::create3DImage(2, 2, 2, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);

    generateFromNoise(0, true, 0.6);

    assignMaterial(occupancyMap);
    assignMaterial(mipMap1);
    assignMaterial(mipMap2);
}

void VoxelWorld::generateFromNoise(double deltaTime, bool isRand2, float fillAmount) // bool isRand2, float fillAmount
{
    this->currentNoiseTime += deltaTime;

    makeNoise(occupancyMap, currentNoiseTime, true, 0.6);
    makeMipMap(occupancyMap, mipMap1);
    makeMipMap(mipMap1, mipMap2);
    // makeMipMap(mipMap2, mipMap3);
    // makeMipMap(mipMap3, mipMap4);
}

void VoxelWorld::bindTextures() const
{
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        occupancyMap, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=1)
        mipMap1, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    glBindImageTexture(
        2, // Image unit index (matches binding=1)
        mipMap2, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    // glBindImageTexture(
    //	3, // Image unit index (matches binding=1)
    //	mipMap3, // Texture ID
    //	0, // Mip level
    //	GL_TRUE, // Layered (true for 3D textures)
    //	0, // Layer (ignored for 3D)
    //	GL_READ_ONLY, // Access qualifier
    //	GL_RGBA8UI // Format
    //);
    //
    // glBindImageTexture(
    //	4, // Image unit index (matches binding=1)
    //	mipMap4, // Texture ID
    //	0, // Mip level
    //	GL_TRUE, // Layered (true for 3D textures)
    //	0, // Layer (ignored for 3D)
    //	GL_READ_ONLY, // Access qualifier
    //	GL_RGBA8UI // Format
    //);
}

void VoxelWorld::unbindTextures() const
{
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    glBindImageTexture(
        2, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    // glBindImageTexture(
    //	3, // Image unit index (matches binding=1)
    //	0, // Texture ID
    //	0, // Mip level
    //	GL_TRUE, // Layered (true for 3D textures)
    //	0, // Layer (ignored for 3D)
    //	GL_READ_ONLY, // Access qualifier
    //	GL_RGBA8UI // Format
    //);
    //
    // glBindImageTexture(
    //	4, // Image unit index (matches binding=1)
    //	0, // Texture ID
    //	0, // Mip level
    //	GL_TRUE, // Layered (true for 3D textures)
    //	0, // Layer (ignored for 3D)
    //	GL_READ_ONLY, // Access qualifier
    //	GL_RGBA8UI // Format
    //);
}

glm::vec3 VoxelWorld::getPosition() const
{
    return position;
}

glm::vec3 VoxelWorld::getScale() const
{
    return scale;
}

glm::quat VoxelWorld::getOrientation() const
{
    return orientation;
}

void VoxelWorld::makeNoise(GLuint image3D, double noiseTime, bool isRand2, float fillAmount)
{
    glUseProgram(makeNoiseComputeProgram);

    // Bind output texture to image unit 1 (write-only)
    glBindImageTexture(
        0, // Image unit index (matches binding=0)
        image3D, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    int outputWidth, outputHeight, outputDepth;

    glBindTexture(GL_TEXTURE_3D, image3D);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &outputWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &outputHeight);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &outputDepth);
    glBindTexture(GL_TEXTURE_3D, 0);

    GLuint workGroupsX = (outputWidth + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (outputHeight + 8 - 1) / 8;
    GLuint workGroupsZ = (outputDepth + 8 - 1) / 8;

    int timeUniform = glGetUniformLocation(makeNoiseComputeProgram, "time");

    glUniform1f(timeUniform, noiseTime);

    glUniform1f(glGetUniformLocation(makeNoiseComputeProgram, "fillAmount"), fillAmount);
    glUniform1i(glGetUniformLocation(makeNoiseComputeProgram, "isRand2"), isRand2);

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );
}

void VoxelWorld::makeMipMap(GLuint inputImage3D, GLuint outputImage3D)
{
    glUseProgram(makeMipMapComputeProgram);

    // Bind output texture to image unit 1 (write-only)
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        inputImage3D, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    // Bind output texture to image unit 1 (write-only)
    glBindImageTexture(
        1, // Image unit index (matches binding=1)
        outputImage3D, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    int outputWidth, outputHeight, outputDepth;

    glBindTexture(GL_TEXTURE_3D, outputImage3D);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &outputWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &outputHeight);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &outputDepth);
    glBindTexture(GL_TEXTURE_3D, 0);

    GLuint workGroupsX = (outputWidth + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (outputHeight + 8 - 1) / 8;
    GLuint workGroupsZ = (outputDepth + 8 - 1) / 8;

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Unbind the images
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );
}

void VoxelWorld::assignMaterial(GLuint image3D)
{
    glUseProgram(assignMaterialComputeProgram);

    // Bind output texture to image unit 1 (write-only)
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        image3D, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA8UI // Format
    );

    int outputWidth, outputHeight, outputDepth;

    glBindTexture(GL_TEXTURE_3D, image3D);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &outputWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &outputHeight);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &outputDepth);
    glBindTexture(GL_TEXTURE_3D, 0);

    GLuint workGroupsX = (outputWidth + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (outputHeight + 8 - 1) / 8;
    GLuint workGroupsZ = (outputDepth + 8 - 1) / 8;

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA8UI // Format
    );
}
