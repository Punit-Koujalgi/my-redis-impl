#ifndef TRANSACTIONHANDLER_H
#define TRANSACTIONHANDLER_H

#include <map>
#include <vector>
#include <string>

class Server;

class TransactionHandler
{
private:

    Server* redisServerPtr{}; // To execute commands in transaction context
    std::map<int, std::vector<std::vector<std::string>>> m_mapClientTransactions; // clientFd, vector of commands in transaction

public:

    TransactionHandler(Server* serverPtr) : redisServerPtr(serverPtr) {}

    std::string AddTransaction(const int clientFd);
    std::string AddCommandToTransaction(const int clientFd, const std::vector<std::string>& command);
    std::string ExecuteTransaction(const int clientFd);
    std::string DiscardTransaction(const int clientFd);
    bool IsInTransaction(const int clientFd);

};


#endif // TRANSACTIONHANDLER_H


