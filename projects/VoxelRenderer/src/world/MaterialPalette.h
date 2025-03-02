#pragma once

#include <cstdint>
#include <src/utilities/NonCopyable.h>
#include <vector>

class MaterialPalette : public NonCopyable
{
public:
    std::vector<uint16_t> materials {};
};
