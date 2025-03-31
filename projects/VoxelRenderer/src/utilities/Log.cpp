#include "Log.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "ColorUtility.h"

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

std::string Log::getLogLevelAnsiColorCode(const LogLevel logLevel)
{
    constexpr int cyan = 0x3cf1e3;
    constexpr int gray = 0xaaaaaa;
    constexpr int red = 0xff0000;
    constexpr int white = 0xffffff;
    constexpr int yellow = 0xfff14b;

    switch (logLevel)
    {
        case Verbose:
        {
            return ColorUtility::ansiForeground(gray);
        }
        case Debug:
        {
            return ColorUtility::ansiForeground(white);
        }
        case Information:
        {
            return ColorUtility::ansiForeground(cyan);
        }
        case Warning:
        {
            return ColorUtility::ansiForeground(yellow);
        }
        case Error:
        {
            return ColorUtility::ansiForeground(red);
        }
        case Fatal:
        {
            return ColorUtility::ansiForeground(white) + ColorUtility::ansiBackground(red);
        }
        default:
        {
            throw std::runtime_error("Unknown log level");
        }
    }
}

std::string Log::getLogLevelText(const LogLevel logLevel)
{
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

void Log::write(const LogLevel logLevel, const std::string& message)
{
    if (logLevel < minimumLevel)
    {
        return;
    }

    std::string timeText = getCurrentTimeText();
    std::string colorCode = getLogLevelAnsiColorCode(logLevel);
    std::string levelText = getLogLevelText(logLevel);

    std::cout << std::format("[{} {}{}{}] {}\n", timeText, colorCode, levelText, ColorUtility::ansiReset(), message) << std::flush;
}
