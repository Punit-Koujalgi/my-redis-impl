#ifndef SUBSCRIPTIONHANDLER_H
#define SUBSCRIPTIONHANDLER_H

#include <memory>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <map>
#include <mutex>
#include <algorithm>

using CommandArray = std::unique_ptr<std::vector<std::string>>;

class Subscription
{
private:
    std::string m_channelName;
    std::vector<int> m_subscribedClients; // list of client fds

public:
    Subscription(const std::string &channelName)
        : m_channelName(channelName) {}

    void AddClient(int clientFd)
    {
        if (std::find(m_subscribedClients.begin(), m_subscribedClients.end(), clientFd) == m_subscribedClients.end())
            m_subscribedClients.push_back(clientFd);
    }

    void RemoveClient(int clientFd)
    {
        if (std::find(m_subscribedClients.begin(), m_subscribedClients.end(), clientFd) == m_subscribedClients.end())
            return;
        m_subscribedClients.erase(std::find(m_subscribedClients.begin(), m_subscribedClients.end(), clientFd));
        // m_subscribedClients.erase(std::remove(m_subscribedClients.begin(), m_subscribedClients.end(), clientFd), m_subscribedClients.end());
    }

    const std::vector<int>& GetSubscribedClients() const
    {
        return m_subscribedClients;
    }

    const std::string& GetChannelName() const
    {
        return m_channelName;
    }
};

class SubscriptionHandler
{
private:
    std::unordered_map<std::string, std::shared_ptr<Subscription>> m_channelSubscriptions;
    std::unordered_map<int, std::vector<std::shared_ptr<Subscription>>> m_clientSubscriptions;

    std::string subscribeHandler(CommandArray commandArgs, const int clientFd);
    std::string publishHandler(CommandArray commandArgs);
    std::string unsubscribeHandler(CommandArray commandArgs, const int clientFd);

public:
    std::string SubscriptionCommandProcessor(CommandArray commandArgs, const int clientFd);
    bool IsClientInSubscribedMode(int clientFd);
    void unsubscribeClientFromAllChannels(int clientFd, bool dontRespond = false);

    ~SubscriptionHandler();
};


#endif


