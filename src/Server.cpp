
#include <iostream>
#include <sys/socket.h> // for socket
#include <arpa/inet.h> 	// for sockaddr_in
#include <unistd.h>		// for read(), close()
#include <cstdlib>
#include <algorithm>
#include <sys/time.h>
#include <fcntl.h>		// for fcntl()

#include "Server.h"
#include "CommandHandler.h"
#include "RESPEncoder.h"
#include "RESPDecoder.h"
#include "Utility.h"
#include "SocketReader.h"
#include "SupportedCommands.h"
#include "StreamHandler.h"
#include <csignal>
#include <cstring>


int Server::signalPipe[2] = {-1, -1};

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

	if (m_mapConfiguration["waitcmd_offset"].empty())
			m_mapConfiguration["waitcmd_offset"] = "0";

	m_dServerFd = socket(AF_INET, SOCK_STREAM, 0);
  	if (m_dServerFd < 0) {
    	throw std::runtime_error("Failed to create server socket");
  	}
  
  	// setting SO_REUSEADDR ensures that we don't run into 'Address already in use' errors
  	int reuse = 1;
  	if (setsockopt(m_dServerFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    	throw std::runtime_error("setsockopt failed");
  	}
  
	// Setup signal handling after socket creation but before event loop	
	setupSignalHandling();

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

#if 0 // test one connection
	int clientfd = acceptNewConnection();
	HandleConnection(clientfd);
#endif

	std::cout << "Starting EventLoop..." << std::endl;

	// initialize my current set
	FD_ZERO(&currentSockets);
	FD_SET(m_dServerFd, &currentSockets);

    // Add signal pipe read end to the socket set
    FD_SET(signalPipe[0], &currentSockets);

	if (getReplicationRole() == "slave")
	{
		/* Add master socket to current sockets to master can send commands */
		FD_SET(m_dMasterConnSocket, &currentSockets);
	}

	while (true)
	{
		readySockets = currentSockets;

		if (select(FD_SETSIZE, &readySockets, nullptr, nullptr, nullptr) < 0)
		{
			if (errno == EINTR)
                continue; // Interrupted by signal, continue

			throw std::runtime_error("select() error or timed out");
		}
		for (int currSock{0}; currSock < FD_SETSIZE; ++currSock)
		{
			if (FD_ISSET(currSock, &readySockets))
			{
				// Check if it's the signal pipe
                if (currSock == signalPipe[0])
				{
                    char buffer[256];
                    ssize_t bytesRead = read(signalPipe[0], buffer, sizeof(buffer));
                    
                    if (bytesRead > 0)
					{
                        std::cout << "Received shutdown signal through pipe" << std::endl;
                        return; // Exit the event loop
                    }
                }

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
					if(bytesRead == -1)
					{
						FD_CLR(currSock, &currentSockets);
						close(currSock);
					}
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
	// This function should not take long => cardinal rule of event loop

	auto commandArgs{SocketReader(clientFd).ReadArray()};
	if (commandArgs.empty())
	{
		std::cout << "Client disconnected: " << clientFd << std::endl;

		/* We fall here whenever the client disconnects 
		   So go place to handle cancelling subscriptions, blocking commands etc
		*/
		m_subscriptionHandler.unsubscribeClientFromAllChannels(clientFd, true); // don't respond to client
		
		return -1;
	}

	std::cout << "Got query: ";
	for (auto& arg : commandArgs) {std::cout << arg << ' ';}
	std::cout << std::endl; 

	std::string status = getReplicationRole();

	auto currentCmd{toLower(commandArgs[0])};
	bool bShouldRespondBack = shouldRespondBack(status, clientFd, commandArgs);

	/* Process the command */
	auto result{HandleCommand(std::make_unique<std::vector<std::string>>(commandArgs), clientFd)};

	if (bShouldRespondBack && result != NO_REPLY)
	{
		//std::cout << "Sending response...[" << result << "]\n";
		std::cout << "Sending response..." << result << std::endl;
		send(clientFd, result.c_str(), result.length(), 0);
	}

	if (status == "master" && shouldPropogateCommand(currentCmd))
		PropogateCommandToReplicas(RESPEncoder::encodeArray(commandArgs));

	if (status == "slave")
	{
		m_mapConfiguration["master_repl_offset"] = std::to_string(std::stoi(m_mapConfiguration["master_repl_offset"])
			+ RESPEncoder::encodeArray(commandArgs).length()); // Keep updating length of processed commands
	}

	if (shouldPropogateCommand(currentCmd)) // A write command: track offset on both master and standby
	{
		m_mapConfiguration["waitcmd_offset"] = std::to_string(std::stoi(m_mapConfiguration["waitcmd_offset"])
			+ RESPEncoder::encodeArray(commandArgs).length()); // Keep updating length of write commands
	}

	return 0;
}

std::string Server::HandleCommand(std::unique_ptr<std::vector<std::string>> ptrArray, const int clientFd /* Replication purposes */)
{
	ptrArray->at(0).assign(toLower(ptrArray->at(0)));

	if (ptrArray->at(0) == MULTI || ptrArray->at(0) == EXEC || ptrArray->at(0) == DISCARD || m_transactionHandler.IsInTransaction(clientFd))
	{
		return CommandHandler::TRANSACTION_cmdHandler(std::move(ptrArray), *this, clientFd);
	}

	if (ptrArray->at(0) == SUBSCRIBE || ptrArray->at(0) == UNSUBSCRIBE || ptrArray->at(0) == PUBLISH || m_subscriptionHandler.IsClientInSubscribedMode(clientFd))
	{
		return CommandHandler::SUBSCRIPTION_cmdHandler(std::move(ptrArray), *this, clientFd);
	}

	if (ptrArray->at(0) == PING)
	{
		return CommandHandler::PING_cmdHandler(std::move(ptrArray));
	}
	else if (ptrArray->at(0) == ECHO)
	{
		return CommandHandler::ECHO_cmdHandler(std::move(ptrArray));
	}
	else if (ptrArray->at(0) == COMMAND)
	{
		return CommandHandler::COMMAND_cmdHandler(std::move(ptrArray));
	}
	else if (ptrArray->at(0) == SET)
	{
		return CommandHandler::SET_cmdHandler(std::move(ptrArray), m_kvStore);
	}
	else if (ptrArray->at(0) == GET)
	{
		return CommandHandler::GET_cmdHandler(std::move(ptrArray), m_kvStore);
	}
	else if (ptrArray->at(0) == CONFIG)
	{
		return CommandHandler::CONFIG_cmdHandler(std::move(ptrArray), m_mapConfiguration);
	}
	else if (ptrArray->at(0) == SAVE)
	{
		// todo: save memory snapshot in rdb format 
	}
	else if (ptrArray->at(0) == KEYS)
	{
		return CommandHandler::KEYS_cmdHandler(std::move(ptrArray), m_kvStore);
	}
	else if (ptrArray->at(0) == INFO)
	{
		return CommandHandler::INFO_cmdHandler(std::move(ptrArray), *this);
	}
	else if (ptrArray->at(0) == REPLCONF)
	{
		return CommandHandler::REPLCONF_cmdHandler(std::move(ptrArray), *this, clientFd);
	}
	else if (ptrArray->at(0) == PSYNC)
	{
		return CommandHandler::PSYNC_cmdHandler(std::move(ptrArray), *this, clientFd);
	}
	else if (ptrArray->at(0) == WAIT)
	{
		return CommandHandler::WAIT_cmdHandler(std::move(ptrArray), *this);
	}
	else if (ptrArray->at(0) == TYPE)
	{
		return CommandHandler::TYPE_cmdHandler(std::move(ptrArray), m_kvStore, m_streamHandler);
	}
	else if (ptrArray->at(0) == XADD || ptrArray->at(0) == XRANGE || ptrArray->at(0) == XREAD)
	{
		return m_streamHandler.StreamCommandProcessor(std::move(ptrArray), clientFd);
	}
	else if (ptrArray->at(0) == INCR)
	{
		return CommandHandler::INCR_cmdHandler(std::move(ptrArray), m_kvStore);
	}
	else if (ptrArray->at(0) == LPOP || ptrArray->at(0) == RPOP || ptrArray->at(0) == LPUSH || ptrArray->at(0) == RPUSH
		|| ptrArray->at(0) == LRANGE || ptrArray->at(0) == LLEN || ptrArray->at(0) == BLPOP)
	{
		return m_listHandler.ListCommandProcessor(std::move(ptrArray), clientFd);
	}
	else
	{
		return RESPEncoder::encodeError("Unsupported command or wrong command format");
	}

	return NULL_BULK_ENCODED; // null bulk string
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
	auto result = SocketReader(m_dMasterConnSocket).readSimpleString();
	if (toLower(result) != toLower("pong"))
		throw std::runtime_error("One step of threeway handshare failed");

	// Step 2a:
	std::vector<std::string> input{"REPLCONF", "listening-port", m_mapConfiguration["port"]};
	sendData(m_dMasterConnSocket, input);
	result = SocketReader(m_dMasterConnSocket).readSimpleString();
	if (toLower(result) != toLower("ok"))
		throw std::runtime_error("Second step of threeway handshare failed");

	// Step 2b:
	std::vector<std::string> input2{"REPLCONF", "capa", "psync2"};
	sendData(m_dMasterConnSocket, input2);
	result = SocketReader(m_dMasterConnSocket).readSimpleString();
	if (toLower(result) != toLower("ok"))
		throw std::runtime_error("Second step of threeway handshare failed");

	// Step 3:
	std::vector<std::string> input3{"PSYNC", "?", "-1"};
	sendData(m_dMasterConnSocket, input3);
	result = SocketReader(m_dMasterConnSocket).readSimpleString();

	// Store Master Information in Configuration
	m_mapConfiguration["masterIP"] = masterIP;
	m_mapConfiguration["masterPort"] = port;
	m_mapConfiguration["master_repl_offset"] = result.substr(result.find_last_of(' ') + 1, 1);

	std::cout << "Threeway handshake complete" << std::endl;

	if (m_mapConfiguration["master_repl_offset"] == "0")
	{
		// Master should be sending empty rdb file now
		result = SocketReader(m_dMasterConnSocket).readRDBFile();
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
				std::cout << "Ignore expected exception: " << "database start not found" << std::endl;
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
		throw std::runtime_error("Failed to send data to master");
}

std::string Server::recvData(const int fd)
{
	/* replaced with socketReader() to read commands one at a time
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
	*/
	return {}; 
}

bool Server::shouldPropogateCommand(const std::string& userCmd)
{
	if (userCmd == SET || userCmd == XADD)
	{
		return true;
	}
	return false;
}

void Server::PropogateCommandToReplicas(const std::string& userCmd)
{
	for (auto& replica : m_mapReplicaPortSocket)
	{
		if (write(replica.second, userCmd.c_str(), userCmd.length()) < 0)
			throw std::runtime_error("Failed to send ping request to replica");

		std::cout << "Cmd Propogated to [replica: " << replica.first << "]\n";
	}
}

bool Server::shouldRespondBack(const std::string& status, const int fd, std::vector<std::string>& args)
{
	if (status == "master"
		|| (status == "slave" && fd != m_dMasterConnSocket) // should always respond to master, below checks only for reponding to master
		|| (status == "slave" && args.size() > 1 && toLower(args[0]) == REPLCONF && toLower(args[1]) == "getack")
		|| (status == "slave" && args.size() > 1 && toLower(args[0]) == COMMAND && toLower(args[1]) == toLower("docs"))) // sent by redis.cli after connecting
	{
		return true;
	}

	return false;
}


void Server::signalHandler(int signal)
{
    std::cout << "\nReceived signal " << signal << ". Initiating graceful shutdown..." << std::endl;
    sendShutdownSignal();
}


void Server::sendShutdownSignal()
{
    if (signalPipe[1] != -1)
	{
        char shutdownMsg = 'Q'; // 'Q' for quit
        ssize_t result = write(signalPipe[1], &shutdownMsg, 1);
        if (result == -1)
		{
            // Don't use std::cerr in signal handler (not async-signal-safe)
            // Just ignore the error or use write() to stderr directly
            std::string msg = "Failed to write to signal pipe\n";
            write(STDERR_FILENO, msg.c_str(), msg.length());
        }
    }
}

void Server::setupSignalHandling()
{
	// Create a pipe for signal communication
	if (pipe(signalPipe) == -1)
	{
		throw std::runtime_error("Failed to create signal pipe: " + std::string(strerror(errno)));
	}

	// Make the write end non-blocking
	int flags = fcntl(signalPipe[1], F_GETFL);
	if (flags == -1 || fcntl(signalPipe[1], F_SETFL, flags | O_NONBLOCK) == -1)
	{
		throw std::runtime_error("Failed to set pipe non-blocking: " + std::string(strerror(errno)));
	}

	// Set up signal handlers
	// std::signal(SIGINT, signalHandler);	 // Ctrl+C
	// std::signal(SIGTERM, signalHandler); // Termination request

	std::cout << "Signal handling setup complete.." << std::endl;
}

void Server::cleanup()
{
	std::cout << "Performing cleanup..." << std::endl;

	// Close all client connections
	for (int fd = 0; fd < FD_SETSIZE; ++fd)
	{
		if (fd != m_dServerFd && fd != m_dMasterConnSocket &&
			fd != signalPipe[0] && fd != signalPipe[1])
		{
			if (fd > 2)
			{ // Don't close stdin/stdout/stderr
				close(fd);
			}
		}
	}

	// Cancel all blocking operations
	// Add these methods when you implement cancellation
	// m_listHandler.cancelAllBlockingOperations();
	// m_streamHandler.cancelAllBlockingOperations();

	// Close master connection if slave
	if (m_dMasterConnSocket != -1)
	{
		close(m_dMasterConnSocket);
		m_dMasterConnSocket = -1;
	}

	// Save state if needed
	// try
	// {
	// 	// Add state saving logic here
	// 	std::cout << "State preservation complete" << std::endl;
	// }
	// catch (const std::exception &e)
	// {
	// 	std::cout << "Warning: Failed to save state: " << e.what() << std::endl;
	// }

	std::cout << "Cleanup complete" << std::endl;
}

Server::~Server()
{
	cleanup();

	if (m_dServerFd != -1)
	{
		close(m_dServerFd);
		m_dServerFd = -1;
	}

	// Close signal pipe
	if (signalPipe[0] != -1)
	{
		close(signalPipe[0]);
		signalPipe[0] = -1;
	}
	if (signalPipe[1] != -1)
	{
		close(signalPipe[1]);
		signalPipe[1] = -1;
	}

	std::cout << "Server Exiting..." << std::endl;
}
