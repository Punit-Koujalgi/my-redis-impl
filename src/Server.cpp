
#include "Server.h"

#include <iostream>
#include <sys/socket.h> // for socket
#include <arpa/inet.h> 	// for sockaddr_in
#include <unistd.h>		// for read(), close()
#include <cstdlib>

Server::~Server()
{
	close(m_dServerFd);
}

void Server::startServer()
{
	m_dServerFd = socket(AF_INET, SOCK_STREAM, 0);
  	if (m_dServerFd < 0) {
    	throw std::runtime_error("Failed to create server socket");
  	}
  
  	// Since the tester restarts your program quite often, setting SO_REUSEADDR
  	// ensures that we don't run into 'Address already in use' errors
  	int reuse = 1;
  	if (setsockopt(m_dServerFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    	throw std::runtime_error("setsockopt failed");
  	}
  
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(6379);
  
	if (bind(m_dServerFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
		 throw std::runtime_error("Failed to bind to port 6379");
	}

	if (listen(m_dServerFd, m_dConnBacklog) != 0) {
		throw std::runtime_error("listen failed");
	}

	std::cout << "Waiting for a client to connect...\n";
}

void Server::runEventLoop()
{
	fd_set currentSockets, readySockets;
	timeval timeout = {120};

#if 0 // test one connection
	int clientfd = acceptNewConnection();
	HandleConnection(clientfd);
#endif

	std::cout << "Starting EventLoop..." << std::endl;

	// initialize my current set
	FD_ZERO(&currentSockets);
	FD_SET(m_dServerFd, &currentSockets);

	while (true)
	{
		readySockets = currentSockets;

		if (select(FD_SETSIZE, &readySockets, nullptr, nullptr, nullptr) < 0)
			throw std::runtime_error("select() error or timed out");

		for (int currSock{0}; currSock < FD_SETSIZE; ++currSock)
		{
			if (FD_ISSET(currSock, &readySockets))
			{
				if (currSock == m_dServerFd)
				{
					// server received a connection
					int clientSocket = acceptNewConnection();
					FD_SET(clientSocket, &currentSockets);
				}
				else
				{
					// One of the client connections has msg
					int bytesRead = HandleConnection(currSock);
					if(bytesRead == 0)
						FD_CLR(currSock, &currentSockets);
				}
			}
		}
	}

}

int Server::acceptNewConnection()
{
	struct sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	int clientFd = accept(m_dServerFd, (struct sockaddr *) &clientAddr, (socklen_t *) &clientAddrLen);
	
	if (clientFd == -1)
		throw std::runtime_error("Accept failed");

	// get IP address
	char clientIp[MAXLINE]{};
	inet_ntop(AF_INET, &clientAddr, clientIp, MAXLINE);

	std::cout << "Client <" << clientIp << "> connected\n";
	return clientFd;
}

int Server::HandleConnection(const int clientFd)
{
	// This function should take long => cardinal rule of event loop

	char buffer[MAXLINE]{};
	auto bytesRead = read(clientFd, buffer, MAXLINE);

	if ( bytesRead < 0)
	{
		throw std::runtime_error("Failed to read data");
	}
	else if (bytesRead == 0)
	{
		std::cout << "No data read" << std::endl;
	}
	else
	{
		std::cout << "Sending response..." << std::endl;
		send(clientFd, "+PONG\r\n", 7, 0);
	}

	return bytesRead;
	// We can also use feof() approach to read multiple lines of data
}