#include "response/FileUtils.hpp"

FileInfo FileUtils::getInfo(const std::string &path)
{
    FileInfo info;
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return info;
    info.exist = true;
    info.isDir = S_ISDIR(st.st_mode);
    info.isFile = S_ISREG(st.st_mode);
    info.isReadable = (access(path.c_str(), R_OK) == 0);
    info.size = static_cast<long>(st.st_size);
    return info;
}


std::string FileUtils::getContent(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    std::stringstream buff;

    if (!file.is_open())
        return ("");
    
    buff << file.rdbuf();
    return buff.str();
}
