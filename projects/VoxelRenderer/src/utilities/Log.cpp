#include "Log.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

std::string Log::getCurrentTimeText()
{
    // Get the current time
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    // Convert to a tm structure
    std::tm buf;
    localtime_r(&in_time_t, &buf);

    // Format the time as HH:MM:SS
    std::ostringstream oss;
    oss << std::put_time(&buf, "%H:%M:%S");
    return oss.str();
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

std::string Log::getLogLevelText(const Log::LogLevel level)
{
    std::string logLevelText;
    switch (level)
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

void Log::write(const LogLevel level, const std::string& message)
{
    if (level < minimumLevel)
    {
        return;
    }

    std::string timeText = getCurrentTimeText();
    std::string levelText = getLogLevelText(level);

    std::cout << std::format("[{} {}] {}\n", timeText, levelText, message) << std::flush;
}
