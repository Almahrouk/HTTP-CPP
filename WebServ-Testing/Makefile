# Compiler and flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 #-fsanitize=address
INCLUDES = -Iinclude

# Directories
SRC_DIR = src
CONFIG_SRC = src/config
SERVER_SRC = src/server
CLIENT_SRC = src/client
RESPONSE_SRC = src/response
CGI_SRC = src/cgi
REQUEST_SRC = src/request
COOKIE_SRC = src/cookie
LOGGER_SRC = src/logger

NAME = webserv

SRCS = main.cpp \
	$(CONFIG_SRC)/Lexer.cpp \
	$(CONFIG_SRC)/Parser.cpp \
	$(CONFIG_SRC)/ParserBlocks.cpp \
	$(CONFIG_SRC)/ParserServerDirectives.cpp \
	$(CONFIG_SRC)/ParserLocationDirectives.cpp \
	$(CONFIG_SRC)/ParserUtils.cpp \
	$(CONFIG_SRC)/Validator.cpp \
	$(CONFIG_SRC)/LocationConfig.cpp \
	$(CONFIG_SRC)/ServerConfig.cpp \
	$(SERVER_SRC)/server.cpp \
	$(SERVER_SRC)/server_client_connection.cpp \
	$(SERVER_SRC)/server_cgi.cpp \
	$(CLIENT_SRC)/Client.cpp \
	$(LOGGER_SRC)/Logger.cpp \
	$(RESPONSE_SRC)/HttpResponse.cpp \
	$(RESPONSE_SRC)/FileUtils.cpp \
	$(CGI_SRC)/Cgi.cpp \
	$(COOKIE_SRC)/Cookie.cpp \
	$(REQUEST_SRC)/HttpRequest.cpp \
	$(REQUEST_SRC)/RequestHandler.cpp \
	$(REQUEST_SRC)/RequestHandlerUtils.cpp

OBJS = $(SRCS:.cpp=.o)

HEADERS = include/config/ConfigException.hpp \
          include/config/Token.hpp \
          include/config/LocationConfig.hpp \
          include/config/ServerConfig.hpp \
          include/config/Lexer.hpp \
          include/config/Parser.hpp \
          include/config/Validator.hpp \
          include/server/server.hpp \
		  include/client/Client.hpp \
		  include/client/CgiClient.hpp \
          include/response/HttpResponse.hpp \
          include/response/FileUtils.hpp \
          include/cgi/Cgi.hpp \
          include/cookie/Cookie.hpp \
          include/request/HttpRequest.hpp \
          include/request/RequestHandler.hpp

GREEN = \033[0;32m
RED = \033[0;31m
YELLOW = \033[0;33m
NC = \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(YELLOW)Linking $(NAME)...$(NC)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)✓ $(NAME) compiled successfully!$(NC)"

%.o: %.cpp $(HEADERS)
	@echo "$(YELLOW)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "$(RED)Cleaning object files...$(NC)"
	@rm -f $(OBJS)
	@echo "$(GREEN)✓ Cleaned!$(NC)"

fclean: clean
	@echo "$(RED)Removing $(NAME)...$(NC)"
	@rm -f $(NAME)
	@echo "$(GREEN)✓ Fully cleaned!$(NC)"

re: fclean all

test: $(NAME)
	@echo "$(YELLOW)Running test...$(NC)"
	@./$(NAME) test.conf

.PHONY: all clean fclean re test