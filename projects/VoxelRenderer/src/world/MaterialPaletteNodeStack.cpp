#include "MaterialPaletteNodeStack.h"

#include <src/utilities/Assert.h>

MaterialPaletteNodeStack::MaterialPaletteNodeStack(const std::shared_ptr<MaterialPaletteNode>& root)
{
    push(root);
}

bool MaterialPaletteNodeStack::canPush() const
{
    return currentIndex + 1 < stack.size();
}

bool MaterialPaletteNodeStack::canPop() const
{
    return currentIndex > 1;
}

void MaterialPaletteNodeStack::push(const std::shared_ptr<MaterialPaletteNode>& node)
{
    Assert::isTrue(canPush(), "Too many nodes in stack");

    currentIndex++;
    stack[currentIndex] = node;
}

void MaterialPaletteNodeStack::pop()
{
    Assert::isTrue(canPop(), "Cannot pop root node");

    stack[currentIndex] = nullptr;
    currentIndex--;
}

std::shared_ptr<MaterialPaletteNode>& MaterialPaletteNodeStack::getCurrent()
{
    return stack[currentIndex];
}

std::span<std::shared_ptr<MaterialPaletteNode>> MaterialPaletteNodeStack::getNodes()
{
    return std::span(stack).subspan(0, currentIndex);
}
