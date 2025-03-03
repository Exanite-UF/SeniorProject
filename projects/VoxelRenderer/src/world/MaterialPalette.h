#pragma once

#include <cstdint>
#include <src/utilities/NonCopyable.h>
#include <vector>

class MaterialPalette : public NonCopyable
{
private:
    std::vector<uint8_t> ids {};

public:
    const std::vector<uint8_t>& getIds();

    void addId(uint8_t id);
    bool hasId(uint8_t id);

    void clear();
};
