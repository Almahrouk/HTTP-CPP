# include "../../include/client/Client.hpp"



Client::Client(const ServerConfig& serverConfig,  int clientPort, std::string clientAddress)
                 : serverConfig(serverConfig), clientPort(clientPort), clientAddress(clientAddress),
                  state(READING_HEADERS), isCgi(false), isCgiResponseError(false), cgi(NULL), bytes_sent(0) {
    request.contentLength = 0;
    error_code = 400;
	start_time = time(NULL);
    timeout = false;
}

Client::~Client()
{
    if (cgi != NULL)
    {
        delete cgi;
        cgi = NULL;
    }
}


void Client::parseRequest()
{
    if (state == READING_HEADERS)
    {
        parseRequestHeaders();
    }

    if (state == READING_BODY)
    {
        parseRequestBody();
    }
}

void    Client::parseRequestHeaders()
{
    size_t  pos = request_str.find("\r\n\r\n");
    if (pos == std::string::npos) {
        return ;
    }
    header_end_pos = pos;

    header_str = request_str.substr(0, header_end_pos);

    std::istringstream header_stream(header_str);

    // READING_HEADERS
    std::string line;
    if (std::getline(header_stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        
        std::istringstream streamLine(line);
        std::string extra;
        if (!(streamLine >> request.method >> request.uri >> request.version) || (streamLine >> extra)) {
            state = ERROR;
            error_code = 400;
            return ;
        };
        if (!(request.method == "GET" || request.method == "POST" || request.method == "DELETE")) {
            state = ERROR;
            error_code = 501;
            return ;
        }
    }
    else {
        state = ERROR;
        error_code = 400;
        return ;
    }

    // parsing headers
    while (std::getline(header_stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break;
        
        size_t  p = line.find(":");
        if (p == std::string::npos) {
            state = ERROR;
            error_code = 400;
            return ;
        }
        std::string key = line.substr(0, p);
        std::string value = line.substr(p + 1);
        key = trim(key);
        value = trim(value);

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        
        request.headers[key] = value;
    }
    if (request.headers.find("content-length") != request.headers.end() ||
        request.headers.find("transfer-encoding") != request.headers.end())
        state = READING_BODY;
    else
        state = COMPLETE;
}

void    Client::parseRequestBody()
{
    if (request.headers.count("transfer-encoding"))
    {
        std::string chunk = request.headers.at("transfer-encoding");
        if (chunk == "chunked")
        {
            std::string body = "";
            std::string req_body = request_str.substr(header_end_pos + 4);
            while (true)
            {
                size_t pos = req_body.find("\r\n");
                if (pos == std::string::npos)
                    return ;

                size_t  length;
                std::string part;
                length = std::strtol(req_body.c_str(), NULL, 16);
                if (length == 0)
                    break ;

                pos += 2;
                req_body = req_body.substr(pos);

                if (req_body.length() < length)
                    return ;

                part = req_body.substr(0, length);
                body.append(part);
                req_body = req_body.substr(length);

                if (req_body.length() < 2)
                    return ;

                if (req_body.substr(0, 2) == "\r\n")
                    req_body = req_body.substr(2);
                else {
                    state = ERROR;
                    error_code = 400;
                    return ;
                }
                
                if (body.size() > serverConfig.getClientMaxBodySize()) {
                    state = ERROR;
                    error_code = 413;
                    return ;
                }
            }

            if (req_body.find("\r\n\r\n") != std::string::npos)
            {
                request.body.clear();
                request.body = body;
                state = COMPLETE;
                return ;
            }
            else {
                return ;
            }
        }
        else {
            state = ERROR;
            error_code = 501;
            return ;
        }
    }
    else if (request.headers.find("content-length") != request.headers.end())
    {
        if (request.method == "GET") {
            state = ERROR;
            return ;
        }
        std::istringstream length_stream(request.headers["content-length"]);
        size_t  length;
        char extra;
        if (!(length_stream >> length) || (length_stream >> extra)) {
            state = ERROR;
            error_code = 400;
            return ;
        }
        size_t client_max_body_size = serverConfig.getClientMaxBodySize();
        if (length > client_max_body_size) {
            state = ERROR;
            error_code = 413;
            return ;
        }

        request.contentLength = length;
        size_t body_start = header_end_pos + 4;
        if (request_str.size() < body_start + request.contentLength) {
            return ;
        }
        request.body.clear();
        request.body = request_str.substr(body_start, request.contentLength);
    }
    state = COMPLETE;
}

std::string trim(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t'))
        start++;

    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
        end--;

    return s.substr(start, end - start);
}

CgiClient::CgiClient(int client_fd, Cgi* cgi, bool is_write) : is_write_end(is_write), http_client_fd(client_fd), cgi(cgi), processed(false) { }