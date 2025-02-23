#pragma once

#include <string>

class Assert
{
public:
    static void isTrue(bool condition, const std::string& errorMessage);
};
