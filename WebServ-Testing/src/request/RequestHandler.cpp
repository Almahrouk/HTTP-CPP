#include "../../include/request/RequestHandler.hpp"
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include "../../include/cgi/Cgi.hpp"
#include "../../include/client/Client.hpp"


HttpResponse RequestHandler::handleRequest(const Client& client)
{
    const HttpRequest& request = client.request;
    const ServerConfig& server = client.serverConfig;

    if (request.contentLength > server.getClientMaxBodySize())
    {
        return makeErrorResponse(413, server);
    }

    const LocationConfig* location = findLocation(request.uri, server);

    if (Logger::isEnabled(Logger::DEBUG))
            Logger::debug("Handling request for URI: " + request.uri);

		
	for (size_t i = 0; i < location->getMethods().size(); i++) {
		if (location->getMethods()[i] == request.method)
			break ;
		if (i == location->getMethods().size() - 1)
			return makeErrorResponse(405, server);
	}
	if (location->getMethods().empty() && (request.method != "GET"))
		return (makeErrorResponse(405, server));
	
    if (request.method == "GET")
        return handleGet(request.uri, server, location);
    else if (request.method == "POST")
        return handlePost(request.uri, request.body, server, location);
    else if (request.method == "DELETE")
        return handleDelete(request.uri, server, location);

    return makeErrorResponse(405, server);
}

const LocationConfig* RequestHandler::findLocation(
    const std::string& uri,
    const ServerConfig& server)
{
    const std::vector<LocationConfig>& locations = server.getLocations();
    const LocationConfig* bestMatch = 0;
    size_t bestLength = 0;

    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string& locPath = locations[i].getPath();
        size_t locLen = locPath.length();

        if (uri.find(locPath) == 0 && locLen > bestLength)
        {
            bestMatch = &locations[i];
            bestLength = locLen;
        }
    }

    return bestMatch;
}

HttpResponse RequestHandler::handleGet(
    const std::string& uri,
    const ServerConfig& server,
    const LocationConfig* location)
{
    if (location && location->hasRedirect())
    {
        return HttpResponse::makeRedireRes(
            location->getRedirectCode(),
            location->getRedirectUrl());
    }

    std::string filePath = buildFilePath(uri, server, location);

    if (directoryExists(filePath))
    {
        if (location && location->getAutoindex())
        {
            if (location->getIndex().empty())
            {
                return HttpResponse::makeAutoindexRes(filePath, uri);
            }
                return HttpResponse::makeFileRes(filePath + "/" + location->getIndex());
        }
        return makeErrorResponse(403, server);
    }
    
    if (!fileExists(filePath))
        return makeErrorResponse(404, server);

    return HttpResponse::makeFileRes(filePath);
}

static std::string extractFilenameFromMultipart(const std::string& body)
{
    // Find Content-Disposition header in multipart body
    std::string contentDisposition = "Content-Disposition: form-data";
    size_t pos = body.find(contentDisposition);
    
    if (pos == std::string::npos)
        return "";
    
    // Find filename="..." after Content-Disposition
    std::string filenameMarker = "filename=\"";
    size_t filePos = body.find(filenameMarker, pos);
    
    if (filePos == std::string::npos)
        return "";
    
    filePos += filenameMarker.length();
    size_t endPos = body.find("\"", filePos);
    
    if (endPos == std::string::npos)
        return "";
    
    return body.substr(filePos, endPos - filePos);
}

static std::string extractFileContentFromMultipart(const std::string& body, const std::string& boundary)
{
    // Find the first boundary
    std::string boundaryStr = "--" + boundary;
    size_t firstBoundary = body.find(boundaryStr);
    
    if (firstBoundary == std::string::npos)
        return body;
    
    // Find the end of headers (double CRLF)
    size_t headerEnd = body.find("\r\n\r\n", firstBoundary);
    if (headerEnd == std::string::npos)
        headerEnd = body.find("\n\n", firstBoundary);
    
    if (headerEnd == std::string::npos)
        return body;
    
    headerEnd += (body[headerEnd] == '\r') ? 4 : 2;
    
    // Find the next boundary (file content ends before it)
    size_t nextBoundary = body.find(boundaryStr, headerEnd);
    
    if (nextBoundary == std::string::npos)
        nextBoundary = body.length();
    
    // Remove trailing CRLF or LF before boundary
    while (nextBoundary > headerEnd && (body[nextBoundary - 1] == '\n' || body[nextBoundary - 1] == '\r'))
        nextBoundary--;
    
    return body.substr(headerEnd, nextBoundary - headerEnd);
}

HttpResponse RequestHandler::handlePost(
    const std::string& uri,
    const std::string& body,
    const ServerConfig& server,
    const LocationConfig* location)
{
	static int suffix = 0;
    if (!location || !location->hasMethod("POST"))
        return makeErrorResponse(405, server);

    if (location->getUploadStore().empty())
        return makeErrorResponse(403, server);

    std::string uploadDir = location->getUploadStore();

    std::string filename = "";
    std::string fileContent = body;

    if (body.find("Content-Disposition") != std::string::npos && body.find("filename=") != std::string::npos)
    {
        std::string boundary = "";
        size_t boundaryPos = body.find("boundary=");
        if (boundaryPos != std::string::npos)
        {
            boundaryPos += 9;
            size_t boundaryEnd = body.find("\r\n", boundaryPos);
            if (boundaryEnd == std::string::npos)
                boundaryEnd = body.find("\n", boundaryPos);
            if (boundaryEnd != std::string::npos)
                boundary = body.substr(boundaryPos, boundaryEnd - boundaryPos);
        }

        if (boundary.empty())
        {
            size_t boundStart = body.find("--");
            if (boundStart != std::string::npos)
            {
                size_t boundEnd = body.find("\r\n", boundStart);
                if (boundEnd == std::string::npos)
                    boundEnd = body.find("\n", boundStart);
                if (boundEnd != std::string::npos)
                    boundary = body.substr(boundStart + 2, boundEnd - boundStart - 2);
            }
        }

        filename = extractFilenameFromMultipart(body);
        fileContent = extractFileContentFromMultipart(body, boundary);
    }

    if (filename.empty())
    {
        std::string uriPath = stripQueryString(uri);
        const std::string& locationPath = location->getPath();
        filename = uriPath;

        if (!locationPath.empty() && uriPath.find(locationPath) == 0)
            filename = uriPath.substr(locationPath.length());

        if (!filename.empty() && filename[0] == '/')
            filename = filename.substr(1);
    }

    if (filename.empty())
    {
        std::ostringstream oss;
        oss << "upload_" << std::time(0) << ++suffix;
        filename = oss.str();
    }

    std::string savePath = joinPath(uploadDir, filename);

    std::ofstream outfile(savePath.c_str(), std::ios::binary);
    if (!outfile.is_open())
        return makeErrorResponse(500, server);

    outfile << fileContent;
    outfile.close();

    HttpResponse response;
    response.setStatus(201, "Created");
    response.setHeaders("Content-Type", "text/plain");
    response.setBody("File uploaded successfully");
    return response;
}


HttpResponse RequestHandler::handleDelete(
    const std::string& uri,
    const ServerConfig& server,
    const LocationConfig* location)
{
    if (location && !location->hasMethod("DELETE"))
        return makeErrorResponse(405, server);

    std::string filePath = buildFilePath(uri, server, location);
    if (Logger::isEnabled(Logger::DEBUG))
        Logger::debug("Resolved delete path: " + filePath);

    if (!fileExists(filePath) || directoryExists(filePath))
        return makeErrorResponse(404, server);

    if (unlink(filePath.c_str()) != 0)
        return makeErrorResponse(500, server);

    HttpResponse response;
    response.setStatus(204, "No Content");
    response.setHeaders("Content-Type", "text/plain");
    response.setBody("");
    return response;
}

// changed
void RequestHandler::handleCgi(Client& client, const LocationConfig* location)
{
    if (!location) {
        client.response = makeErrorResponse(404, client.serverConfig);
        client.isCgiResponseError = true;
        return ;
    }

    if (!location->hasMethod(client.request.method)) {
        client.response = makeErrorResponse(405, client.serverConfig);
        client.isCgiResponseError = true;
        return ;
    }
	for(size_t i = 0; i < location->getMethods().size(); i++) {
		if (location->getMethods()[i] == client.request.method)
			break ;
		if (i == location->getMethods().size() - 1)
		{
			client.response = makeErrorResponse(405, client.serverConfig);
			client.isCgiResponseError = true;
			return ;
		}
	}
    std::string scriptPath = buildFilePath(client.request.uri, client.serverConfig, location);
    if (!fileExists(scriptPath)) {
        client.response = makeErrorResponse(404, client.serverConfig);
        client.isCgiResponseError = true;
        return ;
    }
    if (directoryExists(scriptPath)) {
        client.response = makeErrorResponse(403, client.serverConfig);
        client.isCgiResponseError = true;
        return ;
    }

    HttpRequest cgiRequest;
    if (client.request.headers.find("cookie") != client.request.headers.end())//For cookies
        cgiRequest.headers["cookie"] = client.request.headers.at("cookie");
    cgiRequest.method = client.request.method;
    cgiRequest.uri = client.request.uri;
    cgiRequest.body = client.request.body;
    cgiRequest.contentLength = client.request.body.size();
    cgiRequest.serverName = client.serverConfig.getServerName();
    cgiRequest.serverPort = client.serverConfig.getPort();
    std::ostringstream contentLength;
    contentLength << cgiRequest.contentLength;
    cgiRequest.headers["Content-Length"] = contentLength.str();

    Cgi* cgi_init = new Cgi(cgiRequest, scriptPath, location->getCgiExtensions()); // allocate;
    // assign cgi to client 
    client.cgi = cgi_init;
}