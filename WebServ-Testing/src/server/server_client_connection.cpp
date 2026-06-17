# include "../../include/server/server.hpp"
#include <sstream>


void    Server::acceptClient(int serverFd)
{
    while (true)
    {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        
        memset(&client, 0, len);

        int client_fd = accept(serverFd, reinterpret_cast<sockaddr*>(&client), &len);
        if (client_fd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return ;
            throw std::runtime_error("Failed to add new client accept failed");
        }

        if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
        {
            sendErrorResponse(client_fd, serverFd, client, 500);
            continue ;
        }

        struct pollfd pfd;
        pfd.fd = client_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        // add client to clients & pollFds
        pollFds.push_back(pfd);
        clients.insert(std::make_pair(client_fd, 
                        Client(configs[serverFd], ntohs(client.sin_port),
                        ipToString(client.sin_addr.s_addr))));
        


        std::ostringstream logMessage;
        logMessage << "Accepted client " << ipToString(client.sin_addr.s_addr)
                   << ":" << ntohs(client.sin_port)
                   << " (fd " << pfd.fd << ")";
        Logger::info(logMessage.str());
    }
}

void    Server::readFromClient(int  client_fd)
{
    if (Logger::isEnabled(Logger::DEBUG))
    {
        std::ostringstream readLog;
        readLog << "Reading from client fd " << client_fd;
        Logger::debug(readLog.str());
    }

    char buffer[4096];

    if (clients.find(client_fd) == clients.end()) {
        std::ostringstream missingClient;
        missingClient << "Attempted to read from unknown client fd " << client_fd;
        Logger::warn(missingClient.str());
        return ;
    }

    Client&  client = clients.at(client_fd);
    if (client.isCgi)
    {
        if (Logger::isEnabled(Logger::DEBUG))
        {
            std::ostringstream cgiRead;
            cgiRead << "Client fd " << client_fd << " is already in CGI state";
            Logger::debug(cgiRead.str());
        }
    }

    while (true)
    {
        int n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n > 0)
        {
            client.request_str.append(buffer, n);
            client.parseRequest();

            if (client.state == COMPLETE)
            {
                std::ostringstream requestLog;
                requestLog << client.clientAddress << ":" << client.clientPort
                           << " -> " << client.request.method << " " << client.request.uri;
                Logger::info(requestLog.str());


                const LocationConfig* location = RequestHandler::findLocation(client.request.uri, client.serverConfig);

                if ((client.request.method == "GET" || client.request.method == "POST") &&
                                RequestHandler::isCgiRequest(client.request.uri, location))
                {
					if (!location->hasRedirect()) {
						Logger::info("Handling CGI request");
						RequestHandler::handleCgi(client, location);
						
						if (!client.isCgiResponseError)
						{
							client.cgi->execute();
							
							if (client.cgi->cgi_status == CGI_PIPE_ERROR || client.cgi->cgi_status == CGI_EXEC_ERROR || client.cgi->cgi_status == CGI_ENV_ERROR)
							{
								client.response = RequestHandler::makeErrorResponse(500, client.serverConfig);
								client.response_str = client.response.getResponse();
								changePollEvent(client_fd, POLLOUT);
								return ;
							}

							if (client.request.method == "POST")
								client.cgi->cgi_status = CGI_WRITING;
							else
								client.cgi->cgi_status = CGI_READING;

							try{
								make_cgi_pipes_nonblocking(client.cgi->script_in[1], client.cgi->script_out[0]);
							}
							catch (...) {

								client.cgi->closePipes();
								kill(client.cgi->pid, SIGKILL);
								waitpid(client.cgi->pid, NULL, WNOHANG);
								client.response = RequestHandler::makeErrorResponse(500, client.serverConfig);
								client.response_str = client.response.getResponse();
								changePollEvent(client_fd, POLLOUT);
								return ;
							}
							
							add_cgi_pipes_to_pollFds(client.cgi->script_in[1], client.cgi->script_out[0]);

							// associate cgi pipes to client & .
							cgi_clients.insert(std::make_pair(client.cgi->script_out[0], CgiClient(client_fd, client.cgi, 0)));
							if (client.cgi->script_in[1] != -1 || client.request.method == "POST")
								cgi_clients.insert(std::make_pair(client.cgi->script_in[1], CgiClient(client_fd, client.cgi, 1)));
							
							client.isCgi = true;
							return ;

						}
						else
						{
							client.response_str = client.response.getResponse();
							changePollEvent(client_fd, POLLOUT);
							return ;
						}
					}
                }

                client.response  = RequestHandler::handleRequest(client);
                // Cookies checking
                cookies.checkRequest(client.request);
                if (cookies.shouldSetCookie)
                    client.response.setHeaders(cookies.key, cookies.value);

                client.response_str = client.response.getResponse();

                std::stringstream message;
                message  << "[RESPONSE] status=" << client.response.getStatusCode()
                    << " uri=" << client.request.uri << std::endl;
                Logger::info(message.str());

                changePollEvent(client_fd, POLLOUT);
                return ;
            }
            else if (client.state == ERROR) {
                std::ostringstream errorLog;
                errorLog << "Bad request from " << client.clientAddress << ":" << client.clientPort
                         << " (error " << client.error_code << ")";
                Logger::warn(errorLog.str());

                client.response = RequestHandler::makeErrorResponse(client.error_code, client.serverConfig);
                client.response_str = client.response.getResponse();
                changePollEvent(client_fd, POLLOUT);
                return ;

            }
        }
        else if (n == 0)
        {
            closeClient(client_fd);
            return ;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break ;
            closeClient(client_fd);
            return ;
        }
    }
}

void Server::writeToClient(int  client_fd)
{
    Client& client = clients.at(client_fd);

    if (client.bytes_sent >= client.response_str.size()) {
        closeClient(client_fd);
        return ;
    }

    size_t remaining = client.response_str.size() - client.bytes_sent;

    // size_t chunk_size = std::min(remaining, (size_t)10);

    ssize_t bytes_sent = send(client_fd,
                            client.response_str.c_str() + client.bytes_sent,
                            remaining,
                            0);

    if (bytes_sent > 0)
    {
        if (Logger::isEnabled(Logger::DEBUG))
        {
            std::ostringstream sendLog;
            sendLog << "Sent " << bytes_sent << " bytes to "
                    << client.clientAddress << ":" << client.clientPort;
            Logger::debug(sendLog.str());
        }

        client.bytes_sent += bytes_sent;

        if (Logger::isEnabled(Logger::DEBUG))
        {
            std::ostringstream remainingLog;
            remainingLog << "Remaining bytes to send: "
                         << client.response_str.size() - client.bytes_sent;
            Logger::debug(remainingLog.str());
        }

        if (client.bytes_sent == client.response_str.size())
            closeClient(client_fd);
    }
    else if (bytes_sent == 0)
        closeClient(client_fd);
    else if (bytes_sent < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return ;
        closeClient(client_fd);
    }
}

void    Server::sendErrorResponse(int client_fd, int serverFd, struct sockaddr_in client, int error_code)
{
    clients.insert(std::make_pair(client_fd, 
                        Client(configs[serverFd], ntohs(client.sin_port),
                        ipToString(client.sin_addr.s_addr))));
            
    Client&  http = clients.at(client_fd);

    http.response = RequestHandler::makeErrorResponse(error_code, http.serverConfig);
    http.response_str = http.response.getResponse();

    writeToClient(client_fd);
}

void Server::closeClient(int clientFd)
{
    Client& client = clients.at(clientFd);
    if (client.isCgi && client.cgi != NULL)
    {
        close_cgi_client(client.cgi->script_out[0]);
        close_cgi_client(client.cgi->script_in[1]);
    }

    std::ostringstream closeLog;
    closeLog << "Closed client " << client.clientAddress << ":" << client.clientPort
             << " (fd " << clientFd << ")";
    Logger::info(closeLog.str());

    close(clientFd);

    // remove from pollFds
    for (size_t i = 0; i < pollFds.size(); ++i)
    {
        if (pollFds[i].fd == clientFd)
        {
            pollFds.erase(pollFds.begin() + i);
            break;
        }
    }


    // remove client session
    clients.erase(clientFd);

    clientRemoved = true;
}

std::string ipToString(uint32_t addr)
{
    uint32_t ip = ntohl(addr);
    std::ostringstream oss;
    oss << ((ip >> 24) & 0xFF) << "."
        << ((ip >> 16) & 0xFF) << "."
        << ((ip >> 8) & 0xFF) << "."
        << (ip & 0xFF);
    return oss.str();
}