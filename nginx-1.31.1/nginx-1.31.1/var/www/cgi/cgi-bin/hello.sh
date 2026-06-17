#!/bin/bash

echo ""
echo "Hello from CGI"
echo ""
echo "Request info:"
echo "  Method: ${REQUEST_METHOD}"
echo "  URI: ${REQUEST_URI}"
echo "  Query: ${QUERY_STRING}"
