
#ifndef _SERVER_H_
#define _SERVER_H_

#define MAXLINE 1024

#include <memory>
#include <vector>

class Server
{

public:

	Server() = default;
	~Server();

	void startServer();
	void runEventLoop();

private:
	int acceptNewConnection();
	int HandleConnection(const int clientFd); /* returns bytes read bcoz if 0 then connection must be closed */

	
	std::string HandleCommand(std::unique_ptr<std::vector<std::string>> ptrArray);

private: /* variables */

	int m_dServerFd{-1};
	int m_dConnBacklog{10};
};

#endif