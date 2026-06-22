#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "Common.hpp"

enum LogLevel
{
    INFO, 
    WARNING, 
    ERROR
};

void log(LogLevel level, const std::string& msg);

#endif