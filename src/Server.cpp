
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
#define REPLCONF "replconf"
#define PSYNC "psync"

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

	if (getReplicationRole() == "slave")
	{
		/* Add master socket to current sockets to master can send commands */
	}

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
		auto result{HandleCommand(std::move(commandArgs), clientFd)};

		//std::cout << "Sending response...[" << result << "]\n";
		std::cout << "Sending response..." << result << std::endl;
		send(clientFd, result.c_str(), result.length(), 0);
	}

	return bytesRead;
	// We can also use feof() approach to read multiple lines of data
}

std::string Server::HandleCommand(std::unique_ptr<std::vector<std::string>> ptrArray, const int clientFd /* Replication purposes */)
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
	else if (ptrArray->at(0) == REPLCONF)
	{
		if (ptrArray->at(1) == "listening-port")
		{
			m_mapReplicaPortSocket[ptrArray->at(1)] = clientFd;
			std::cout << "Got Replica connection [port: " << ptrArray->at(1) << "]" << std::endl;
		}

		return "+OK\r\n";
	}
	else if (ptrArray->at(0) == PSYNC)
	{
		std::string result = "FULLRESYNC " +
				m_mapConfiguration["master_replid"] + " " +
				m_mapConfiguration["master_repl_offset"] + "\r\n";

		result = RESPEncoder::encodeSimpleString(result);

		std::cout << "Sending response..." << std::endl;
		send(clientFd, result.c_str(), result.length(), 0);

		/* Send initial empty rdb file */
		const std::string empty_rdb = "\x52\x45\x44\x49\x53\x30\x30\x31\x31\xfa\x09\
\x72\x65\x64\x69\x73\x2d\x76\x65\x72\x05\x37\x2e\x32\x2e\x30\xfa\x0a\x72\x65\
\x64\x69\x73\x2d\x62\x69\x74\x73\xc0\x40\xfa\x05\x63\x74\x69\x6d\x65\xc2\x6d\
\x08\xbc\x65\xfa\x08\x75\x73\x65\x64\x2d\x6d\x65\x6d\xc2\xb0\xc4\x10\x00\xfa\
\x08\x61\x6f\x66\x2d\x62\x61\x73\x65\xc0\x00\xff\xf0\x6e\x3b\xfe\xc0\xff\x5a\xa2";

		std::cout << "Sending response..." << std::endl;
		send(clientFd, std::string("$" + std::to_string(empty_rdb.length()) + "\r\n").c_str(), 5, 0);

		//return "$" + std::to_string(empty_rdb.length()) + "\r\n" + empty_rdb;	
		return empty_rdb;
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

	m_dMasterConnSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_dMasterConnSocket < 0)
		throw std::runtime_error("Failed to create master conn socket");

	std::string masterStr = m_mapConfiguration["replicaof"]; // "<master IP> <master port>"
	std::string masterIP = masterStr.substr(0, masterStr.find(' '));
	if (masterIP == "localhost")
		masterIP = "127.0.0.1"; // todo: write utiity to convert host to ip
	std::string port = masterStr.substr(masterStr.find(' ') + 1);

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(stoi(port));
	
	if (inet_pton(AF_INET, masterIP.c_str(), &servaddr.sin_addr) <= 0)
		throw std::runtime_error("Failed to convert IP to network format");
	
	if (connect(m_dMasterConnSocket, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		throw std::runtime_error("Failed to connect to master");

	// Connected!! Threeway handshake underway
	// Step 1:
	sendData(m_dMasterConnSocket, {PING});
	auto result = recvData(m_dMasterConnSocket);
	if (toLower(*RESPDecoder::decodeString(result)) != toLower("pong"))
		throw std::runtime_error("One step of threeway handshare failed");

	// Step 2a:
	std::vector<std::string> input{"REPLCONF", "listening-port", m_mapConfiguration["port"]};
	sendData(m_dMasterConnSocket, input);
	result = recvData(m_dMasterConnSocket);
	if (toLower(*RESPDecoder::decodeString(result)) != toLower("ok"))
		throw std::runtime_error("Second step of threeway handshare failed");

	// Step 2b:
	std::vector<std::string> input2{"REPLCONF", "capa", "psync2"};
	sendData(m_dMasterConnSocket, input2);
	result = recvData(m_dMasterConnSocket);
	if (toLower(*RESPDecoder::decodeString(result)) != toLower("ok"))
		throw std::runtime_error("Second step of threeway handshare failed");

	// Step 3:
	std::vector<std::string> input3{"PSYNC", "?", "-1"};
	sendData(m_dMasterConnSocket, input3);
	result = *RESPDecoder::decodeString(recvData(m_dMasterConnSocket));

	// Store Master Information in Configuration
	m_mapConfiguration["masterIP"] = masterIP;
	m_mapConfiguration["masterPort"] = port;
	m_mapConfiguration["masterOffset"] = result.substr(result.find_last_of(' ') + 1, 1);

	std::cout << "Threeway handshake complete" << std::endl;

	if (m_mapConfiguration["masterOffset"] == "0")
	{
		// Master should be sending empty rdb file now
		result = recvData(m_dMasterConnSocket);
		result = result.substr(result.find('\n') + 1);
		createFileWithData("/tmp/emptyDb.rdb", result); /* Even if data is empty we can still info like version, metadata etc */
		try
		{
			m_kvStore.initializeKeyValues("/tmp", "emptyDb.rdb");
			std::cout << "Initialized from Master's rdb file" << std::endl;
		}
		catch(const std::exception& e)
		{
			if (std::string_view(e.what()).find("database start") != std::string::npos)
			{
				// Expected no database exception bcoz we are initializing from empty rdb file
				std::cout << "Ignore exception: " << e.what() << std::endl;
			}
			else
				throw; // re-throw any other exception
		}
	}
	else
	{
		throw std::runtime_error("Initializing from full master data state not supported yet");
	}
}

void Server::sendData(const int fd, const std::vector<std::string>& vec)
{
	std::string pingReq = RESPEncoder::encodeArray(vec);

	if (write(fd, pingReq.c_str(), pingReq.length()) < 0)
		throw std::runtime_error("Failed to send ping request to master");
}

std::string Server::recvData(const int fd)
{
	std::string result;
	char recvline[MAXLINE]{};

	int bytesRead = read(fd, recvline, MAXLINE - 1);
	if (bytesRead < 0)
		throw std::runtime_error("Failed to receive response from master");

	result.append(recvline);

	if (result.ends_with("\r\n"))
		result = result.substr(0, result.length() - 2);

	std::cout << "Received data: " << result << std::endl;
	return result;
}


