#include "Log.h"

#include <iostream>

void Log::log(const std::string& value)
{
    std::cout << value + "\n"
              << std::flush;
}
