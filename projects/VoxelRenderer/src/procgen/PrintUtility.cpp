#include <src/procgen/PrintUtility.h>

#include <iostream>
#include <fstream>

void PrintUtility::printTexture(std::shared_ptr<TextureData>& textureData, std::function<float(float)> mapTo01, const std::string& filename)
{
    int width = textureData->getSize().x;
    int height = textureData->getSize().y;
    
    std::ofstream file(filename);
    if(file.is_open())
    {
        file << "P3\n" << width << " " << height << "\n255\n";

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float sample = mapTo01(textureData->get(x, y));
                int colorValue = static_cast<int>(textureData->get(x, y) * 255);
                colorValue = std::max(0, std::min(255, colorValue));
                file << colorValue << " " << colorValue << " " << colorValue << " ";
            }
            file << "\n";
        }

        file.close();
        std::cout << "Texture output to " << filename << std::endl;
    } 
    else 
    {
        std::cerr << "Error opening file: " << filename << std::endl;
    }
}
