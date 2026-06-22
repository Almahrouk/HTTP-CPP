#include "Logger.hpp"
#include <iostream>

void log(LogLevel level, const std::string& msg) {
    std::ostream& out = (level == ERROR) ? std::cerr : std::cout;
    switch (level) {
        case INFO:    out << "[INFO]    "; break;
        case WARNING: out << "[WARNING] "; break;
        case ERROR:   out << "[ERROR]   "; break;
    }
    out << msg << std::endl;
}