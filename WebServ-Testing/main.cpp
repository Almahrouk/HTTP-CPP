#include <iostream>
#include "include/config/Lexer.hpp"
#include "include/config/Parser.hpp"
#include "include/config/Validator.hpp"
#include "include/server/server.hpp"

int main(int ac, char **av)
{
	if (ac != 2)
	{
		std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
		return 1;
	}

	try
	{
		// std::cout << "==========================================\n" << std::endl;

		Lexer lexer(av[1]);

		Parser parser(lexer.getTokens());
		parser.parse();
		Logger::info("Configuration parsed successfully");

		Validator validator(parser.getServers());
		validator.validate();
		Logger::info("Configuration validated successfully");

		Server		server(parser.getServers());
		server.run();

		return 0;
	}
	catch (std::exception& e)
	{
		std::cerr << "\n✗ Error: " << e.what() << std::endl;
		return 1;
	}
}
