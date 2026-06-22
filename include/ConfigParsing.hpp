#ifndef _CONFIGPARSING_HPP
#define _CONFIGPARSING_HPP
#include "Common.hpp"

enum ConfigParsingError
{
    NOT_Valid_Exten = 1,
    File_Too_Long,
    File_Access_FAILURE,
    File_Read_FAILURE
};

class ConfigParsing
{
    private:
        int _num_of_servers;
        std::string _config_file_name;
        std::fstream _file;
        int _num_of_tokens;
        std::vector<Token> tokens;
        bool Is_Valid_File_Extention();
        bool File_Path_Or_Name_Too_Long();
        bool Is_Accessable();
        bool Is_Readable();
        int Tokenization();
        // int Parsing(); 
    public:
        ConfigParsing();
        ConfigParsing(const std::string &config_file_name);
        ~ConfigParsing();
        int Check_File();
        void Display_File_Content();
        int Tokenization();

};

enum Token_Char
{
    WORD,
    OPEN_CURLY_BRACE,
    CLOSE_CURLY_BRACE,
    SEMICOLON,
    SPACE,
    HASH,
    DEFAULT
};

class Token
{
    private:
        std::string str;
        int index;
        Token* next;
        Token* prev;
        enum Token_Char;
    public:
        Token();
        ~Token();
};

enum Directive_Type
{
    SIMPLE,
    BLOCK, 
    CONTEXT
};

class Directive
{
    private:
        bool Is_Simple_Directive();
        std::string _key;
        std::vector<std::string> _parameters; // values
        std::vector<Directive> _context_directive;
        
    public:
        Directive();
        ~Directive();
};


#endif