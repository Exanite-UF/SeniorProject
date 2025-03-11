#pragma once

#include <array>
#include <memory>
#include <src/Constants.h>
#include <src/world/MaterialPaletteNode.h>

class MaterialPaletteNodeStack
{
private:
    int currentIndex = -1;
    std::array<std::shared_ptr<MaterialPaletteNode>, Constants::VoxelWorld::paletteMapLayerCount + 1> stack;

public:
    explicit MaterialPaletteNodeStack(const std::shared_ptr<MaterialPaletteNode>& root);

    [[nodiscard]] bool canPush() const;
    [[nodiscard]] bool canPop() const;

    void push(const std::shared_ptr<MaterialPaletteNode>& node);
    void pop();

    int getCount() const;
    int getCurrentIndex();
    std::shared_ptr<MaterialPaletteNode>& getCurrent();

    std::span<std::shared_ptr<MaterialPaletteNode>> getNodes();
};
