#include "Log.h"

#include <iostream>

void Log::information(const std::string& value)
{
    std::cout << value + "\n"
              << std::flush;
}
