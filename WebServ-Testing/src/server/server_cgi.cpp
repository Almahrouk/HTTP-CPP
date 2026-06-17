# include "../../include/server/server.hpp"



void    Server::processCgiEvent(int cgi_pipe)
{
    CgiClient& cgi_client = cgi_clients.at(cgi_pipe);

    if (cgi_client.is_write_end)
        processCgiWriteEvent(cgi_pipe);
    else
        processCgiReadEvent(cgi_pipe);
}

void    Server::make_cgi_pipes_nonblocking(int& script_in, int& script_out)
{
    if (fcntl(script_out, F_SETFL, O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to make the client cgi script_out nonblocking");
    }
    
    if (script_in != -1 && fcntl(script_in, F_SETFL, O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to make the client cgi script_in nonblocking");
    }
}

void    Server::add_cgi_pipes_to_pollFds(int script_in, int script_out)
{
    struct pollfd pfd;
    pfd.fd = script_out;
    pfd.events = POLLIN;
    pfd.revents = 0;
    pollFds.push_back(pfd);

    if (script_in != -1)
    {
        struct pollfd pfd_write;
        pfd_write.fd = script_in;
        pfd_write.events = POLLOUT;
        pfd_write.revents = 0;
        pollFds.push_back(pfd_write);
    }
}

void    Server::processCgiReadEvent(int cgi_pipe)
{
    CgiClient& cgi_client = cgi_clients.at(cgi_pipe);

    if (cgi_client.processed)
    {
        std::ostringstream alreadyProcessed;
        alreadyProcessed << "CGI already processed for pipe " << cgi_pipe;
        Logger::debug(alreadyProcessed.str());

        changePollEvent(cgi_pipe, POLLIN);
        return ;
    }
    if (cgi_client.cgi == NULL)
        return ;

    if (clients.find(cgi_client.http_client_fd) == clients.end())
        return ;

    
    cgi_client.cgi->read_output();

    if (cgi_client.cgi->cgi_status == CGI_DONE_READING)
    {
        cgi_client.cgi->build_response();
        setHttpClientResponse(*cgi_client.cgi, cgi_client.http_client_fd);
        changePollEvent(cgi_client.http_client_fd, POLLOUT);

        // remove from poll
                    for (size_t i = 0; i < pollFds.size(); ++i)
                    {
                        if (pollFds[i].fd == cgi_pipe)
                        {
                            pollFds.erase(pollFds.begin() + i);
                            clientRemoved = true;
                            break ;
                        }
                    }
        cgi_client.processed = true;
    }
}

void    Server::processCgiWriteEvent(int cgi_pipe)
{
    CgiClient& cgi_client = cgi_clients.at(cgi_pipe);

    if (cgi_client.processed)
        return ;
    if (cgi_client.cgi == NULL)
        return ;

    if (clients.find(cgi_client.http_client_fd) == clients.end())
    {
        std::ostringstream clientClosed;
        clientClosed << "HTTP client closed for CGI pipe " << cgi_pipe
                     << " (client fd " << cgi_client.http_client_fd << ")";
        Logger::warn(clientClosed.str());
        return ;
    }

    cgi_client.cgi->write_body();
    
    if (cgi_client.cgi->cgi_status == CGI_DONE_WRITING)
    {
        cgi_client.cgi->cgi_status = CGI_READING;
        cgi_client.processed = true;
        // remove from poll
                    for (size_t i = 0; i < pollFds.size(); ++i)
                    {
                        if (pollFds[i].fd == cgi_pipe)
                        {
                            pollFds.erase(pollFds.begin() + i);
                            clientRemoved = true;
                        }
                    }
        
        close(cgi_client.cgi->script_in[1]);
        cgi_client.cgi->script_in[1] = -1;
        cgi_clients.erase(cgi_pipe);

        std::ostringstream doneWriting;
        doneWriting << "CGI done writing for pipe " << cgi_pipe;
        Logger::info(doneWriting.str());
    }
}


void    Server::close_cgi_client(int fd)
{
    if (fd == -1)
        return ;
    std::ostringstream closeLog;
    closeLog << "Closing CGI client fd " << fd;
    Logger::info(closeLog.str());
    
    CgiClient& cgi_client = cgi_clients.at(fd);

    kill(cgi_client.cgi->pid, SIGKILL);
    waitpid(cgi_client.cgi->pid, NULL, WNOHANG);

    close(fd);

    // remove from poll
    for (size_t i = 0; i < pollFds.size(); ++i)
    {
        if (pollFds[i].fd == fd)
        {
            pollFds.erase(pollFds.begin() + i);
            break ;
        }
    }

    // remove from cgi_clients map
    cgi_clients.erase(fd);
    
    clientRemoved = true;
}

bool    Server::isCgiPipe(int fd)
{
    return (cgi_clients.find(fd) != cgi_clients.end());
}
