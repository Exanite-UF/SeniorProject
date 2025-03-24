#pragma once

#include <atomic>
#include <string>

class Log
{
    enum LogLevel
    {
        Verbose = 0,
        Debug = 1,
        Information = 2,
        Warning = 3,
        Error = 4,
        Fatal = 5,
    };

public:
    inline static std::atomic<LogLevel> minimumLevel = Information;

    static void verbose(const std::string& value);
    static void debug(const std::string& value);
    static void information(const std::string& value);
    static void warning(const std::string& value);
    static void error(const std::string& value);
    static void fatal(const std::string& value);

    static void write(LogLevel level, const std::string& value);
};
