#include "Log.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

std::string Log::getCurrentTimeText()
{
    // Get the current time
    auto time = std::time(nullptr);
    auto tm = *std::localtime(&time);

    // Format the time as HH:MM:SS
    std::ostringstream stream;
    stream << std::put_time(&tm, "%H:%M:%S");
    return stream.str();
}

void Log::verbose(const std::string& message)
{
    write(Verbose, message);
}

void Log::debug(const std::string& message)
{
    write(Debug, message);
}

void Log::information(const std::string& message)
{
    write(Information, message);
}

void Log::warning(const std::string& message)
{
    write(Warning, message);
}

void Log::error(const std::string& message)
{
    write(Error, message);
}

void Log::fatal(const std::string& message)
{
    write(Fatal, message);
}

std::string Log::getLogLevelText(const Log::LogLevel logLevel)
{
    std::string logLevelText;
    switch (logLevel)
    {
        case Verbose:
        {
            return "VRB";
        }
        case Debug:
        {
            return "DBG";
        }
        case Information:
        {
            return "INF";
        }
        case Warning:
        {
            return "WRN";
        }
        case Error:
        {
            return "ERR";
        }
        case Fatal:
        {
            return "FTL";
        }
        default:
        {
            throw std::runtime_error("Unknown log level");
        }
    }
}

void Log::write(const LogLevel logLevel, const std::string& message)
{
    if (logLevel < minimumLevel)
    {
        return;
    }

    std::string timeText = getCurrentTimeText();
    std::string levelText = getLogLevelText(logLevel);

    std::cout << std::format("[{} {}] {}\n", timeText, levelText, message) << std::flush;
}
