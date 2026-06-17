# include "../../include/server/server.hpp"
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>




Server::Server(const std::vector<ServerConfig>& servers)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        int         port = servers[i].getPort();
        std::string host = servers[i].getHost();
        int         newServerSocket = addListeningSocket(port, host);

        configs.insert(std::make_pair(newServerSocket, servers[i])); 

        std::ostringstream logMessage;
        logMessage << "Listening on " << host << ":" << port << " (fd " << newServerSocket << ")";
        Logger::info(logMessage.str());
    }
    clientRemoved = false;
}

void    Server::bindSocket(int newServerSocket, int port, std::string host)
{
    struct sockaddr_in nic;


    memset(&nic, 0, sizeof(nic));
    nic.sin_family = AF_INET;
    nic.sin_port = htons(port);

    if (host == "0.0.0.0")
        nic.sin_addr.s_addr = htonl(INADDR_ANY);
    else
    {
        struct addrinfo  addrType;
        struct addrinfo* res = 0;

        memset(&addrType, 0, sizeof(addrType));
        addrType.ai_family = AF_INET;
        addrType.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(host.c_str(), 0, &addrType, &res) != 0 || res == 0)
            throw std::runtime_error("Failed to bind socket with port " + intToString(port));
        
        nic.sin_addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
        freeaddrinfo(res);
    }

    if (bind(newServerSocket, reinterpret_cast<sockaddr*>(&nic), sizeof(nic)) == -1)
        throw std::runtime_error("Failed to bind socket with port " + intToString(port));
}

int    Server::addListeningSocket(int port, std::string host)
{
    int newServSocket;

    newServSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (newServSocket < 0)
        throw std::runtime_error("Failed to create listening socket port : " + intToString(port));
    
    int opt = 1;
    if (setsockopt(newServSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(newServSocket);
        throw std::runtime_error("Failed to set socket options " + intToString(port));
    }

    bindSocket(newServSocket, port, host);

    if (listen(newServSocket, SOMAXCONN) == -1) {
        close(newServSocket);
        throw std::runtime_error("Failed to listen on socket binded to port : " + intToString(port));
    }

    if (fcntl(newServSocket, F_SETFL, O_NONBLOCK) == -1) {
        close(newServSocket);
        throw std::runtime_error("Failed to make the socket non Blocking" + intToString(port));
    }

    struct pollfd pfd;
    pfd.fd = newServSocket;
    pfd.events = POLLIN;
    pfd.revents = 0;

    listenSockets.push_back(newServSocket);
    pollFds.push_back(pfd);

    return (newServSocket);
}

void    Server::run()
{
    while (1337)
    {
        int ret = poll(pollFds.data(), pollFds.size(), 6000);

        if (ret < 0)
            throw std::runtime_error("poll failed can't listen on servers sockets");

        if (Logger::isEnabled(Logger::DEBUG))
        {
            std::ostringstream pollLog;
            pollLog << "Polling cycle started (pollFds size: " << pollFds.size() << ")";
            Logger::debug(pollLog.str());
        }

        // CGI: timeout check
        Logger::debug("Checking clients Timeout ");
        std::map<int, Client>::iterator it;
        for (it = clients.begin(); it != clients.end(); ++it)
        {
			if (!it->second.isCgi && !it->second.timeout && time(NULL) - it->second.start_time >= TIMEOUT)
			{
				std::stringstream   message;
                message  << "[HTTP_TIMEOUT] port=" << it->second.clientPort << " Sending error response & closing client " << std::endl;
                Logger::warn(message.str());

                it->second.timeout = true;
				it->second.response = RequestHandler::makeErrorResponse(504, it->second.serverConfig);
                it->second.response_str = it->second.response.getResponse();
                changePollEvent(it->first, POLLOUT);
				// writeToClient(it->first);
			}

            else if (it->second.isCgi && it->second.cgi != NULL && it->second.cgi->checkTimeout())
            {
                std::stringstream   message;
                message  << "[CGI_TIMEOUT] pid=" << it->second.cgi->pid << "Sending error response & closing client " << std::endl;
                Logger::warn(message.str());

                it->second.response = RequestHandler::makeErrorResponse(504, it->second.serverConfig);
                it->second.response_str = it->second.response.getResponse();
                changePollEvent(it->first, POLLOUT);

            }
        }

        for (size_t i = 0; i < pollFds.size();)
        {
            if (pollFds[i].revents == 0) {
                ++i;
                continue ;
            }
            clientRemoved = false;

            if (pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (isCgiPipe(pollFds[i].fd)) {
                    if (pollFds[i].revents & POLLHUP)
                        processCgiEvent(pollFds[i].fd);
                }
                else {
                    closeSocket(pollFds[i].fd);
                    continue ;
                }
            }

            // check for client read event
            if (!clientRemoved && pollFds[i].revents & POLLIN)
            {
                if (isListeningSocket(pollFds[i].fd))
                    acceptClient(pollFds[i].fd);
                
                else if (isCgiPipe(pollFds[i].fd))
                    processCgiReadEvent(pollFds[i].fd);
                else
                    readFromClient(pollFds[i].fd);
            }

            // check for client write event
            if (!clientRemoved && (pollFds[i].revents & POLLOUT))
            {
                if (isCgiPipe(pollFds[i].fd))
                    processCgiWriteEvent(pollFds[i].fd);
                else
                    writeToClient(pollFds[i].fd);
            }

            
            if (!clientRemoved)
                ++i;
        }

        
    }
}

bool Server::isListeningSocket(int fd)
{
    return std::find(listenSockets.begin(),
                     listenSockets.end(),
                     fd) != listenSockets.end();
}

void    Server::closeSocket(int fd)
{
    std::ostringstream closeLog;
    closeLog << "Closing socket fd " << fd;
    Logger::info(closeLog.str());

    if (isListeningSocket(fd))
    {
        configs.erase(fd);

        for (size_t i = 0; i < listenSockets.size(); ++i)
        {
            if (listenSockets[i] == fd)
            {
                listenSockets.erase(listenSockets.begin() + i);
                break;
            }
        }
    }
	
    else
    {
        if (clients.find(fd) == clients.end())
            return ;
        Client& client = clients.at(fd);
        if (client.isCgi)
        {
            close_cgi_client(client.cgi->script_out[0]);
            close_cgi_client(client.cgi->script_in[1]);
        }
        clients.erase(fd);
    }

    close(fd);

    // remove from pollFds
    for (size_t i = 0; i < pollFds.size(); ++i)
    {
        if (pollFds[i].fd == fd)
        {
            pollFds.erase(pollFds.begin() + i);
            break;
        }
    }

}

void    Server::changePollEvent(int fd, short int event)
{
    for (size_t i = 0; i < pollFds.size(); ++i)
    {
        if (pollFds[i].fd == fd)
        {
            pollFds[i].revents = 0;
            pollFds[i].events = event;
            break ;
        }
    }
}

void    Server::setHttpClientResponse(Cgi& cgi, int http_client_fd)
{
    // Cookies checking
    cookies.checkRequest(clients.at(http_client_fd).request);
    if (cookies.shouldSetCookie)
        cgi.res.setHeaders(cookies.key, cookies.value);

    clients.at(http_client_fd).response_str = cgi.getResponse();
}

std::string intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}