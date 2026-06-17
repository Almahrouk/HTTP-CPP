# pragma once

# include <string>
# include <sstream>
# include <iostream>
# include "../config/Parser.hpp"
# include "../request/HttpRequest.hpp"
# include "../response/HttpResponse.hpp"
# include "../cgi/Cgi.hpp"
# include <poll.h>



class CgiClient
{
public:
    bool            is_write_end;
    int             http_client_fd;
    Cgi*            cgi;
    bool            processed;

    CgiClient(int client_fd, Cgi* cgi, bool is_write);

};
