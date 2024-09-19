
#ifndef _SERVER_H_
#define _SERVER_H_

#define MAXLINE 1024

class Server
{

public:

	Server() = default;
	~Server();

	void startServer();
	void runEventLoop();

	int acceptNewConnection();
	void HandleConnection(const int clientFd);

private:

	int m_dServerFd{-1};
	int m_dConnBacklog{10};
};

#endif