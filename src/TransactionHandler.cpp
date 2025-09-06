
#include "Server.h"
#include "TransactionHandler.h"
#include "RESPEncoder.h"
#include <iostream>

std::string TransactionHandler::AddTransaction(const int clientFd)
{
    if (IsInTransaction(clientFd))
        return RESPEncoder::encodeError("MULTI calls can not be nested");

    m_mapClientTransactions[clientFd] = std::vector<std::vector<std::string>>{};
    return RESPEncoder::encodeSimpleString("OK");
}

std::string TransactionHandler::AddCommandToTransaction(const int clientFd, const std::vector<std::string> &command)
{
    if (m_mapClientTransactions.find(clientFd) == m_mapClientTransactions.end())
        return RESPEncoder::encodeError("No transaction started for this client");

    m_mapClientTransactions[clientFd].emplace_back(command);
    std::cout << "Adding command of length " << command.size() << " to transaction for client " << clientFd << std::endl;
    return RESPEncoder::encodeSimpleString("QUEUED");
}

std::string TransactionHandler::ExecuteTransaction(const int clientFd)
{
    if (!IsInTransaction(clientFd))
        return RESPEncoder::encodeError("EXEC without MULTI");

    std::cout << "Executing " << m_mapClientTransactions[clientFd].size() << " commands in transaction for client " << clientFd << std::endl;

    auto commands = m_mapClientTransactions[clientFd];
    std::vector<std::string> results;

    m_mapClientTransactions.erase(clientFd);  // Clear the transaction before execution so they are not queued again

    for (auto command : commands)
    {
        auto result = redisServerPtr->HandleCommand(std::make_unique<std::vector<std::string>>(command), clientFd);
        results.push_back(result);
    }

    return RESPEncoder::encodeArray(results, true); // true to avoid double encoding
}

std::string TransactionHandler::DiscardTransaction(const int clientFd)
{
    if (m_mapClientTransactions.erase(clientFd) > 0)
    {
        // Transaction discarded
        return RESPEncoder::encodeSimpleString("OK");
    }

    return RESPEncoder::encodeError("DISCARD without MULTI");
}

bool TransactionHandler::IsInTransaction(const int clientFd)
{
    return m_mapClientTransactions.find(clientFd) != m_mapClientTransactions.end();
}
