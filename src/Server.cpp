
#include "Server.h"
#include "RESPEncoder.h"
#include "RESPDecoder.h"
#include "Utility.h"

#include <iostream>
#include <sys/socket.h> // for socket
#include <arpa/inet.h> 	// for sockaddr_in
#include <unistd.h>		// for read(), close()
#include <cstdlib>
#include <algorithm>

/* Supported commands */
#define PING "ping"
#define ECHO "echo"
#define COMMAND "command"
#define SET "set"
#define GET "get"
#define CONFIG "config"
#define SAVE "save"
#define KEYS "keys"
#define INFO "info"

Server::~Server()
{
	close(m_dServerFd);
}

void Server::startServer(int argc, char **argv)
{
	for (int index{0}; index < argc ; ++index)
	{
		std::string arg = argv[index];
		if (arg.length() > 2 && arg[0] == '-' && arg[1] == '-')
		{
			std::string key = arg.substr(2);
			if (index + 1 < argc)
			{
				std::string_view value = argv[++index];
				m_mapConfiguration[std::move(key)] = std::move(value);
			}
		}
	}

	// Initialize from rdb file if it's present
	m_kvStore.initializeKeyValues(m_mapConfiguration["dir"], m_mapConfiguration["dbfilename"]);

	if (getReplicationRole() == "master")
	{
		m_mapConfiguration["master_replid"] = "8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb";
		m_mapConfiguration["master_repl_offset"] = "0";
	}

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
  
	if (m_mapConfiguration["port"].empty())	
			m_mapConfiguration["port"] = "6379";
	std::cout << "Redis port: " << m_mapConfiguration["port"] << std::endl;

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(stoi(m_mapConfiguration["port"]));
  
	if (bind(m_dServerFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
		 throw std::runtime_error("Failed to bind to port 6379");
	}

	if (listen(m_dServerFd, m_dConnBacklog) != 0) {
		throw std::runtime_error("listen failed");
	}

	if (getReplicationRole() == "slave")
	{
		initializeSlave();
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
		std::cout << "Got query [" << buffer << "]. Decoding..." << std::endl;

		auto commandArgs(RESPDecoder::decodeArray(std::move(buffer)));
		auto result{HandleCommand(std::move(commandArgs))};

		//std::cout << "Sending response...[" << result << "]\n";
		std::cout << "Sending response..." << std::endl;
		send(clientFd, result.c_str(), result.length(), 0);
	}

	return bytesRead;
	// We can also use feof() approach to read multiple lines of data
}

std::string Server::HandleCommand(std::unique_ptr<std::vector<std::string>> ptrArray)
{
	ptrArray->at(0).assign(toLower(ptrArray->at(0)));
	
	if (ptrArray->at(0) == PING)
	{
		return "+PONG\r\n";
	}
	else if (ptrArray->at(0) == ECHO)
	{
		return "+" + ptrArray->at(1) + "\r\n";
	}
	else if (ptrArray->at(0) == COMMAND)
	{
		return "+OK\r\n";
	}
	else if (ptrArray->at(0) == SET)
	{
		if (ptrArray->size() == 5  && toLower(ptrArray->at(3)) == "px")
		{
			return m_kvStore.set(ptrArray->at(1), ptrArray->at(2), stoi(ptrArray->at(4)));
		}
		else if (ptrArray->size() == 3)
		{
			return m_kvStore.set(ptrArray->at(1), ptrArray->at(2));
		}
	}
	else if (ptrArray->at(0) == GET)
	{
		return m_kvStore.get(ptrArray->at(1));
	}
	else if (ptrArray->at(0) == CONFIG)
	{
		if (toLower(ptrArray->at(1)) == "get" &&
				m_mapConfiguration.find(ptrArray->at(2)) != m_mapConfiguration.end())
		{

			return RESPEncoder::encodeArray({ptrArray->at(2), m_mapConfiguration[ptrArray->at(2)]});
		}
	}
	else if (ptrArray->at(0) == SAVE)
	{
		// todo: save memory snapshot in rdb format 
	}
	else if (ptrArray->at(0) == KEYS)
	{
		return RESPEncoder::encodeArray(*m_kvStore.getAllKeys(ptrArray->at(1)));
	}
	else if (ptrArray->at(0) == INFO)
	{
		if (ptrArray->at(1) == toLower("replication"))
		{
			std::string role = getReplicationRole();
			std::string result = "role:" + role + "\n";

			if (role == "master")
			{
				result.append("master_replid:");
				result.append(m_mapConfiguration["master_replid"]);
				result.append("\n");
				result.append("master_repl_offset:");
				result.append(m_mapConfiguration["master_repl_offset"]);
				result.append("\n");
			}

			return RESPEncoder::encodeString(result);
		}
	}


	return "$-1\r\n"; // null bulk string
}

std::string Server::getReplicationRole()
{
	if (m_mapConfiguration.find("replicaof") != m_mapConfiguration.end())
	{
		std::string masterStr = m_mapConfiguration["replicaof"];
		std::string master = masterStr.substr(masterStr.find(' ') + 1);

		std::cout << "This is Slave [Master: " << master << "]\n";
		return "slave";
	}

	return "master";
}

void Server::initializeSlave()
{
	/* Performs three way handshake with master */

	int masterConnSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (masterConnSocket < 0)
		throw std::runtime_error("Failed to create master conn socket");

	std::string masterStr = m_mapConfiguration["replicaof"]; // "<master IP> <master port>"
	std::string masterIP = masterStr.substr(0, masterStr.find(' '));
	std::string port = masterStr.substr(masterStr.find(' ') + 1);

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(stoi(port));
	
	if (inet_pton(AF_INET, masterIP.c_str(), &servaddr.sin_addr) <= 0)
		throw std::runtime_error("Failed to convert IP to network format");
	
	if (connect(masterConnSocket, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		throw std::runtime_error("Failed to connect to master");

	// Connected!!

	std::string pingReq = RESPEncoder::encodeArray({"PING"});

	if (write(masterConnSocket, pingReq.c_str(), pingReq.length()) < 0)
		throw std::runtime_error("Failed to send ping request to master");

	char recvline[MAXLINE]{};

	while(true)
	{
		int bytesRead = read(masterConnSocket, recvline, MAXLINE - 1);
		if (bytesRead < 0)
			throw std::runtime_error("Failed to receive response from master");
		else if (bytesRead == 0)
			break;

		std::cout << recvline;
	}

}


