#include "Log.h"

#include <iostream>

void Log::verbose(const std::string& value)
{
    write(Verbose, value);
}

void Log::debug(const std::string& value)
{
    write(Debug, value);
}

void Log::information(const std::string& value)
{
    write(Information, value);
}

void Log::warning(const std::string& value)
{
    write(Warning, value);
}

void Log::error(const std::string& value)
{
    write(Error, value);
}

void Log::fatal(const std::string& value)
{
    write(Fatal, value);
}

void Log::write(const LogLevel level, const std::string& value)
{
    if (level < minimumLevel)
    {
        return;
    }

    std::cout << value + "\n"
              << std::flush;
}
