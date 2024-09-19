
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

	/* returns bytes read bcoz if 0 then connection must be closed */
	int HandleConnection(const int clientFd);

private:

	int m_dServerFd{-1};
	int m_dConnBacklog{10};
};

#endif