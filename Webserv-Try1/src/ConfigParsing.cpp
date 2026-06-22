#include "ConfigParsing.hpp"
#include "Logger.hpp"
#include <fstream>

ConfigParsing::ConfigParsing()
    : _num_of_servers(0) : _num_of_tokens = 0 : tokens(nullptr)
{
}

ConfigParsing::ConfigParsing(const std::string &config_file_name)
    : _num_of_servers(0), _config_file_name(config_file_name)
{
    printf("ConfigParsing Constructor Called\n");
    printf("File Name: %s\n", this->_config_file_name.c_str());
}

ConfigParsing::~ConfigParsing()
{
}

int ConfigParsing::Check_File()
{
    printf("Start Check File...\n");
    if (!Is_Valid_File_Extention())
    {
        printf("Is NOT Valid File Extention\n");
        log(ERROR, "Not valid file name. Must end with .conf");
        return NOT_Valid_Exten;
    }
    if (File_Path_Or_Name_Too_Long())
    {
        printf("File Path or Name Is Too Long\n");
        log(ERROR, "File path or name too long");
        return File_Too_Long;
    }
    _file.open(this->_config_file_name.c_str(), std::ios::in);
    if (!Is_Accessable())
    {
        printf("File Can NOT Access\n");
        log(ERROR, "Cannot Access configuration file");
        return File_Access_FAILURE;
    }
    if (!Is_Readable())
    {
        printf("File Can NOT Read\n");
        log(ERROR, "Cannot Read configuration file");
        _file.close();
        return File_Read_FAILURE;
    }
    _file.close();
    return EXIT_SUCCESS;
}

bool ConfigParsing::Is_Valid_File_Extention()
{
    if (this->_config_file_name.size() < 5)
        return false;
    return this->_config_file_name.substr(this->_config_file_name.size() - 5) == ".conf";
}

bool ConfigParsing::File_Path_Or_Name_Too_Long()
{
    return this->_config_file_name.size() >= 1000;
}

bool ConfigParsing::Is_Accessable()
{
    return _file.is_open();
}

bool ConfigParsing::Is_Readable()
{
    std::string line;
    return static_cast<bool>(getline(_file, line));
}

void ConfigParsing::Display_File_Content()
{
    _file.seekg(0);
    std::string line;
    while (getline(_file, line))
        std::cout << line << std::endl;
    if (_file.eof())
        printf("Reached end of file.\n");
    else
        printf("Error: File reading failed!\n");
}

int ConfigParsing::Tokenization()
{
    _file.seekg(0);
    std::string line;
    this->_num_of_tokens = 0;
    while (getline(_file, line))
    {
        std::cout << line << std::endl;
        // TODO
        // Remove_Spaces(); // while is NOT sapce
        // Is_Comment(); // if the first char is #, the line is just a comment -> ignore line: use continue;
        // split line to tokens by spaces -> each word is a Token
        // spicify the setting of the Token
    }
    if (_file.eof())
        printf("Reached end of file.\n");
    else
        printf("Error: File reading failed!\n");
}

// nested key-value config parser
int ConfigParsing::Parsing()
{
    // std::stack<Node*>   stack;
    // TODO
    // 1. from Token to Directive
        //  Directive is a Stack Strcuture (push pop)
        // loop on Tokens:
            // push to stack
            // 
            // if token_pos == 0: the first node --> root key
            // value of it ...
            // it is simple || block Directive
    // 
}

Token::Token()
{
    str = "";
    position = -1;
    next =  nullptr;
    prev =  nullptr;
    Token_Char = DEFAULT;
}

Token::~Token()
{
}