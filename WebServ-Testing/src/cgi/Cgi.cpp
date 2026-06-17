#include "../../include/cgi/Cgi.hpp"

Cgi::Cgi() : status(0), envp(NULL), pid(-1), cgi_status(CGI_IDLE), start_time(0), timeout(0), written(0)
{
    script_in[0] = -1;
    script_in[1] = -1;
    script_out[0] = -1;
    script_out[1] = -1;
};

Cgi::Cgi(const HttpRequest &req, const std::string &scriptPath, const std::map<std::string, std::string> &cgiExtension, time_t timeout)
    : status(0), envp(NULL), pid(-1), cgi_status(CGI_IDLE), start_time(0)
    , method(req.method), timeout(timeout), script_path(scriptPath), cgi_extensions(cgiExtension)
    , written(0), req_body(req.body)
{
    script_in[0] = -1;
    script_in[1] = -1;
    script_out[0] = -1;
    script_out[1] = -1;

    buildEnvp(req);
    if (!envp || cgi_status == CGI_ENV_ERROR)
    {
        res = HttpResponse::makeErrorRes(500, "");
        return;
    }
}

void    Cgi::build_response()
{
    if (output.empty())
    {
        cgi_status = CGI_EXEC_ERROR;
        Logger::error("CGI failed to execute script: " + script_path);
        makeResponse();
        return;
    }
    cgi_status = CGI_SUCCESS;
    parseOutput(output);
    makeResponse();
}

Cgi::~Cgi()
{
    free_envp();
}


void Cgi::closePipes()
{
    if (script_in[0] != -1)
    {
        close(script_in[0]);
        script_in[0] = -1;
    }
    if (script_in[1] != -1)
    {
        close(script_in[1]);
        script_in[1] = -1;
    }
    if (script_out[0] != -1)
    {
        close(script_out[0]);
        script_out[0] = -1;
    }
    if (script_out[1] != -1)
    {
        close(script_out[1]);
        script_out[1] = -1;
    }
}

void Cgi::free_envp()
{
    if (!envp)
        return;
    for (size_t i = 0; envp[i] != NULL; i++)
        delete [] envp[i];
    delete [] envp;
    envp = NULL;
}

std::string Cgi::ultostr(size_t num)
{
    std::stringstream ss;
    ss << num;
    return ss.str();
}

void    Cgi::buildHeaders(const HttpRequest &req)
{
    std::map<std::string , std::string>::const_iterator it = req.headers.begin();
    const std::string http_prefix = "HTTP_";
    while (it != req.headers.end())
    {
        std::string key = it->first;
        for (size_t i = 0; i < key.size(); i++)
        {
            if (key[i] == '-')
                key[i] = '_';
            else
             key[i] = toupper(key[i]);
        }
        env_vect.push_back(http_prefix + key + "=" + it->second);
        it++;
    }
}


void    Cgi::parseOutputHeaders(const std::string &output)
{
    size_t pos = 0;
    while (pos < output.size())
    {
        size_t end = output.find("\r\n", pos);
        if (end != std::string::npos)
        {
            std::string line = output.substr(pos, end - pos);
            if (line.empty())
                break;
            size_t colon = line.find(':');
            if (colon != std::string::npos)
            {
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                    value.erase(0, 1);
                headers[key] = value;
            }
            pos = end + 2;
        }
        else
            break;
    }
}

void Cgi::parseOutput(const std::string &output)
{
    size_t pos = output.find("\r\n\r\n");
    if (pos != std::string::npos)
    {
        parseOutputHeaders(output.substr(0, pos + 2));
        body = output.substr(pos + 4);
    }
    else
        body = output;
}

void Cgi::makeResponse()
{
    if (cgi_status != CGI_SUCCESS)
    {
        res = HttpResponse::makeErrorRes(500, "");
        return;
    }
    res = HttpResponse::makeCgiRes(body, headers);
}

void Cgi::execute()
{
    if (pipe(script_in) == -1)
    {
        cgi_status = CGI_PIPE_ERROR;
        return;
    }
    if (pipe(script_out) == -1)
    {
        cgi_status = CGI_PIPE_ERROR;
        close(script_in[0]);
        close(script_in[1]);
        return;
    }

    pid = fork();
    if (pid == -1)
    {
        cgi_status = CGI_EXEC_ERROR;
        closePipes();
        return;
    }
    if (pid == 0)
    {
        signal(SIGPIPE, SIG_IGN);
        dup2(script_in[0], STDIN_FILENO);
        dup2(script_out[1], STDOUT_FILENO);
        closePipes();
        char *argv[] = {
            const_cast<char *>(script_interpreter.c_str()),
            const_cast<char *>(script_path.c_str()),
            NULL
        };
        execve(script_interpreter.c_str(), argv, envp);
        free_envp();
        _exit(1);
    }
    close(script_in[0]);
    close(script_out[1]);
    start_time = time(NULL);

    if (method == "GET")
    {
        close(script_in[1]);// we dont need to write to the cgi child
        script_in[1] = -1;
    }
}

void    Cgi::read_output()
{
    if (cgi_status == CGI_READING)
    {
        if (Logger::isEnabled(Logger::DEBUG))
            Logger::debug("Reading output from CGI child");

        char buff[4096];
        read_bytes = read(script_out[0], buff, sizeof(buff));
        if (read_bytes > 0)
            output.append(buff, read_bytes);
        else if (read_bytes == 0)
        {
            if (Logger::isEnabled(Logger::DEBUG))
                Logger::debug("CGI finished reading output");

            cgi_status = CGI_DONE_READING;
            return ;
        }
        else if (read_bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return ;
    }
}

void    Cgi::write_body()
{
    if (cgi_status == CGI_WRITING)
    {
        int write_bytes;
        write_bytes = write(script_in[1], req_body.c_str() + written, req_body.size() - written);
        if (write_bytes > 0)
            written += write_bytes;
        else if (written == req_body.size())
        {
            cgi_status = CGI_DONE_WRITING;
            return ;
        }
        else if (write_bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return ;
    }
}


void    Cgi::buildEnvp(const HttpRequest &req)
{
    size_t pos = script_path.find_last_of('/');
    script_name = (pos != std::string::npos) ? script_path.substr(pos + 1) : script_path;
    pos = req.uri.find('?');
    request_uri = (pos != std::string::npos) ? req.uri.substr(0, pos) : req.uri;
    query_string = (pos != std::string::npos) ? req.uri.substr(pos + 1) : "";
    std::string content_type = req.headers.count("Content-Type") ? req.headers.at("Content-Type") : "text/plain";

    env_vect.push_back("REQUEST_METHOD=" + req.method);
    env_vect.push_back("REQUEST_URI=" + request_uri);
    env_vect.push_back("CONTENT_TYPE=" + content_type);
    env_vect.push_back("CONTENT_LENGTH=" + ultostr(req.contentLength));
    env_vect.push_back("QUERY_STRING=" + query_string);
    env_vect.push_back("SCRIPT_FILENAME=" + script_path);
    env_vect.push_back("SCRIPT_NAME=" + script_name);
    env_vect.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env_vect.push_back("SERVER_PROTOCOL=HTTP/1.0");
    env_vect.push_back("SERVER_SOFTWARE=WebServ/1.0");
    env_vect.push_back(("SERVER_PORT=" + ultostr(req.serverPort)));
    env_vect.push_back("SERVER_NAME=" + req.serverName);
    if (req.headers.count("cookie"))
        env_vect.push_back("COOKIE=" + req.headers.at("cookie"));

    buildHeaders(req);

    pos = script_name.find_last_of('.');
    std::string ext = (pos != std::string::npos) ? script_name.substr(pos) : "";
    if (cgi_extensions.count(ext))
        script_interpreter = cgi_extensions.at(ext);
    else
    {
        cgi_status = CGI_ENV_ERROR;
        Logger::error("CGI unsupported script type: " + ext);
        
        return;
    }

    envp = new char*[env_vect.size() + 1];
    for (size_t i = 0; i < env_vect.size(); i++)
    {
        envp[i] = new char[env_vect[i].size() + 1];
        std::strcpy(envp[i], env_vect[i].c_str());
    }
    envp[env_vect.size()] = NULL;
}



bool Cgi::checkTimeout()
{
    if (cgi_status != CGI_READING && cgi_status != CGI_WRITING)
        return false;
    time_t curr = time(NULL);
    if (difftime(curr, start_time) > timeout)
    {
        cgi_status = CGI_TIMEOUT;
        kill(pid, SIGKILL);
        waitpid(pid, &status, WNOHANG);
        return true;
    }
    return false;
}


std::string Cgi::getResponse() const
{
    return res.getResponse();
}
