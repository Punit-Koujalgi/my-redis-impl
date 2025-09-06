
#ifndef _SERVER_H_
#define _SERVER_H_

#define MAXLINE 1024

#include <memory>
#include <vector>
#include <map>

#include "KeyValueStore.h"
#include "StreamHandler.h"
#include "TransactionHandler.h"
#include "ListHandler.h"

class Server
{

public:

	Server() : m_transactionHandler(this) {}
	~Server();

	void startServer(int argc, char **argv);
	void runEventLoop();

private:
	int acceptNewConnection();
	int HandleConnection(const int clientFd); /* returns bytes read bcoz if 0 then connection must be closed */

	
	std::string HandleCommand(std::unique_ptr<std::vector<std::string>> ptrArray, const int clientFd /* Replication purposes */);
	std::string getReplicationRole();
	void initializeSlave();
	void PropogateCommandToReplicas(const std::string& userCmd);
	bool shouldPropogateCommand(const std::string& userCmd);
	bool shouldRespondBack(const std::string& status, const int fd, std::vector<std::string>& args);

	void sendData(const int fd, const std::vector<std::string>& vec);
	std::string recvData(const int fd); 

private: /* variables */

	KeyValueStore m_kvStore;
	StreamHandler m_streamHandler;
	TransactionHandler m_transactionHandler;
	ListHandler m_listHandler;

	std::unordered_map<std::string, std::string> m_mapConfiguration;
	std::map<std::string, int> m_mapReplicaPortSocket;

	int m_dServerFd{-1};
	int m_dMasterConnSocket{-1};
	int m_dConnBacklog{10};

	friend class CommandHandler;
	friend class TransactionHandler;
};

#endif

