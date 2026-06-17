#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "common.hpp"

enum LogLevel
{
    INFO, 
    WARNING, 
    ERROR
};

void log(LogLevel level, const std::string& msg);

#endif