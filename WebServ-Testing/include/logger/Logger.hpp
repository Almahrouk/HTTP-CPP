#pragma once

#include <string>
#include <ostream>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>


class Logger
{
public:
    enum Level
    {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    static void     setLevel(Level level);
    static Level    getLevel();
    static bool     isEnabled(Level level);

    static void log(Level level, const std::string& message);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static Level currentLevel;

    static std::string timestamp();
    static const char* levelLabel(Level level);
    static std::ostream& streamFor(Level level);
};
