
#include <iostream>
#include "Server.h"

#define MAXLINE 1024

int main(int argc, char **argv) {
	// Flush after every std::cout / std::cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	try
	{
		Server tcpServer;

		tcpServer.startServer(argc, argv);
		tcpServer.runEventLoop();

	}
	catch(const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
	}
	catch(...)
	{
		std::cerr << "Error: Unhandled exception occurred :(" << '\n';
	}

	std::cout << "Redis OUT!" << std::endl;

	return 0;
}

