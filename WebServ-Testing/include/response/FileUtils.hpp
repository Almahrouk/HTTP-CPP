#pragma once

#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

struct FileInfo
{
    bool exist;
    bool isFile;
    bool isDir;
    bool isReadable;
    long size;

    FileInfo() : exist(false), isFile(false), isDir(false),
                 isReadable(false), size(-1) {}
};

class   FileUtils
{
public:
    static FileInfo getInfo(const std::string &path);
    static std::string getContent(const std::string &path);
};