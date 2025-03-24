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

private:
    static std::string getCurrentTimeText();
    static std::string getLogLevelText(LogLevel level);

public:
    inline static std::atomic<LogLevel> minimumLevel = Verbose;

    static void verbose(const std::string& message);
    static void debug(const std::string& message);
    static void information(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    static void fatal(const std::string& message);

    static void write(LogLevel level, const std::string& message);
};
