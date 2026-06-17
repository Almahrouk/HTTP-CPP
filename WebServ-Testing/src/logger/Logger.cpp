#include "../../include/logger/Logger.hpp"


Logger::Level Logger::currentLevel = Logger::INFO;

void Logger::setLevel(Level level)
{
    currentLevel = level;
}

Logger::Level Logger::getLevel()
{
    return currentLevel;
}

bool Logger::isEnabled(Level level)
{
    return level >= currentLevel;
}

void Logger::log(Level level, const std::string& message)
{
    if (!isEnabled(level))
        return;
    std::ostream& out = streamFor(level);
    out << "[" << timestamp() << "] "
        << levelLabel(level) << " "
        << message << std::endl;
}

void Logger::debug(const std::string& message)
{
    log(DEBUG, message);
}

void Logger::info(const std::string& message)
{
    log(INFO, message);
}

void Logger::warn(const std::string& message)
{
    log(WARN, message);
}

void Logger::error(const std::string& message)
{
    log(ERROR, message);
}

std::string Logger::timestamp()
{
    std::time_t now = std::time(NULL);
    std::tm* local = std::localtime(&now);
    std::ostringstream oss;

    if (!local)
        return "unknown-time";

    oss << std::setfill('0')
        << std::setw(4) << (local->tm_year + 1900) << "-"
        << std::setw(2) << (local->tm_mon + 1) << "-"
        << std::setw(2) << local->tm_mday << " "
        << std::setw(2) << local->tm_hour << ":"
        << std::setw(2) << local->tm_min << ":"
        << std::setw(2) << local->tm_sec;

    return oss.str();
}

const char* Logger::levelLabel(Level level)
{
    switch (level)
    {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARN: return "WARN";
        case ERROR: return "ERROR";
        default: return "INFO";
    }
}

std::ostream& Logger::streamFor(Level level)
{
    if (level == ERROR || level == WARN)
        return std::cerr;
    return std::cout;
}