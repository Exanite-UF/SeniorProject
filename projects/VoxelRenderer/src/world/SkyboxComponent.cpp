#include "SkyboxComponent.h"

#include <src/graphics/TextureManager.h>
#include <src/utilities/Assert.h>

#include <fstream>
#include <iostream>

SkyboxComponent::SkyboxComponent(const std::string& cubemapFilePath, const std::string& skyboxSettingFilePath)
{
    cubemap = TextureManager::getInstance().loadCubemapTexture(cubemapFilePath, GL_RGB16F);

    std::ifstream settingFile(skyboxSettingFilePath);
    Assert::isTrue(settingFile.is_open(), "Failed to load skybox setting file: " + skyboxSettingFilePath);

    int lineCounter = 0;
    std::string line;
    while (std::getline(settingFile, line)) { // Read each line into the 'line' variable

        switch(lineCounter){
            case 0:
                sunDirection.x = std::stof(line);
                break;
            case 1:
                sunDirection.y = std::stof(line);
                break;
            case 2:
                sunDirection.z = std::stof(line);
                break;
            case 3:
                sunAngularSize = std::stof(line);
                break;
            case 4:
                sunBrightnessMultiplier = std::stof(line);
                break;
            case 5:
                skyBrightnessMultiplier = std::stof(line);
                break;
            case 6:
                visualMultiplier = std::stof(line);
                break;
            default:
                break;
        }

        lineCounter++;

        if(lineCounter == 7){
            break;//This is the last line with relevant info
        }
    }
    Assert::isTrue(lineCounter == 7, "Failed to load all skybox settings: " + skyboxSettingFilePath);


    settingFile.close();  // Close the file when done
}

std::shared_ptr<Texture> SkyboxComponent::getCubemap()
{
    return cubemap;
}

float SkyboxComponent::getSunAngularSize() const
{
    return sunAngularSize;
}

float SkyboxComponent::getSunBrightnessMultiplier() const
{
    return sunBrightnessMultiplier;
}

float SkyboxComponent::getSkyBrightnessMultiplier() const
{
    return skyBrightnessMultiplier;
}

float SkyboxComponent::getVisualMultiplier() const
{
    return visualMultiplier;
}

glm::vec3 SkyboxComponent::getSunDirection() const
{
    return sunDirection;
}
