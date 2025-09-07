
#include "SubscriptionHandler.h"
#include "RESPEncoder.h"
#include "Utility.h"
#include "SupportedCommands.h"
#include <sys/socket.h>
#include <iostream>

std::string SubscriptionHandler::SubscriptionCommandProcessor(CommandArray commandArgs, const int clientFd)
{
    if (!commandArgs || commandArgs->empty())
        throw std::runtime_error("Invalid command Array");

    commandArgs->at(0).assign(toLower(commandArgs->at(0)));

    std::string &command = (*commandArgs)[0];

    if (command == "subscribe")
    {
        return subscribeHandler(std::move(commandArgs), clientFd);   
    }
    else if (command == "unsubscribe")
    {
        if (commandArgs->size() == 1)
        {
            unsubscribeClientFromAllChannels(clientFd);
            return NO_REPLY;
        }
        return unsubscribeHandler(std::move(commandArgs), clientFd);
    }
    else if (command == "ping")
    {
        std::vector<std::string> respArray {"pong", ""};
        return RESPEncoder::encodeArray(respArray);
    }
    else if (command == "publish")
    {
        return publishHandler(std::move(commandArgs));
    }

    return RESPEncoder::encodeError("Unsupported subscription command");
}


bool SubscriptionHandler::IsClientInSubscribedMode(int clientFd)
{
    return m_clientSubscriptions.find(clientFd) != m_clientSubscriptions.end();
}

std::string SubscriptionHandler::subscribeHandler(CommandArray commandArgs, const int clientFd)
{
    for (size_t i = 1; i < commandArgs->size(); ++i)
    {
        std::string &channelName = (*commandArgs)[i];
        if (m_channelSubscriptions.find(channelName) == m_channelSubscriptions.end())
        {
            m_channelSubscriptions[channelName] = std::make_shared<Subscription>(channelName);
        }
        m_channelSubscriptions[channelName]->AddClient(clientFd);
        m_clientSubscriptions[clientFd].emplace_back(m_channelSubscriptions[channelName]);

        // Send subscription confirmation to client
        std::vector<std::string> respArray = {RESPEncoder::encodeString("subscribe"),
            RESPEncoder::encodeString(channelName),
            RESPEncoder::encodeInteger(static_cast<int>(m_clientSubscriptions[clientFd].size()))};

        std::string response = RESPEncoder::encodeArray(respArray, true);
        send(clientFd, response.c_str(), response.length(), 0);

        std::cout << "Client " << clientFd << " subscribed to channel " << channelName << std::endl;
    }
    
    return NO_REPLY;
}

std::string SubscriptionHandler::unsubscribeHandler(CommandArray commandArgs, const int clientFd)
{
    for (size_t i = 1; i < commandArgs->size(); ++i)
    {
        std::string &channelName = (*commandArgs)[i];
        if (m_channelSubscriptions.find(channelName) != m_channelSubscriptions.end())
        {
            m_channelSubscriptions[channelName]->RemoveClient(clientFd);
            if (m_channelSubscriptions[channelName]->GetSubscribedClients().empty())
            {
                m_channelSubscriptions.erase(channelName);
                std::cout << "Channel " << channelName << " has no more subscribers and is removed!" << std::endl;
            }
        }
        if (m_clientSubscriptions.find(clientFd) != m_clientSubscriptions.end())
        {
            auto &subs = m_clientSubscriptions[clientFd];
            subs.erase(std::remove_if(subs.begin(), subs.end(),
                    [&channelName](const std::shared_ptr<Subscription> &sub) { return sub->GetSubscribedClients().empty() || sub->GetChannelName() == channelName; }),
                subs.end());

            if (subs.empty())
            {
                m_clientSubscriptions.erase(clientFd);
                std::cout << "Client " << clientFd << " has no more subscriptions!" << std::endl;
            }
        }

        // Send unsubscription confirmation to client
        std::vector<std::string> respArray = {RESPEncoder::encodeString("unsubscribe"),
            RESPEncoder::encodeString(channelName),
            RESPEncoder::encodeInteger(m_clientSubscriptions.find(clientFd) != m_clientSubscriptions.end() ?
                static_cast<int>(m_clientSubscriptions[clientFd].size()) : 0)};
            
        std::string response = RESPEncoder::encodeArray(respArray, true);
        send(clientFd, response.c_str(), response.length(), 0);

        std::cout << "Client " << clientFd << " unsubscribed from channel " << channelName << std::endl;
    }

    return NO_REPLY;
}

void SubscriptionHandler::unsubscribeClientFromAllChannels(int clientFd, bool dontRespond)
{
    if (m_clientSubscriptions.find(clientFd) == m_clientSubscriptions.end())
        return;

    auto &subs = m_clientSubscriptions[clientFd];
    int channelsSubscribed = static_cast<int>(subs.size());

    for (const auto &sub : subs)
    {
        sub->RemoveClient(clientFd);
        if (sub->GetSubscribedClients().empty())
        {
            m_channelSubscriptions.erase(sub->GetChannelName());
            std::cout << "Channel " << sub->GetChannelName() << " has no more subscribers and is removed!" << std::endl;
        }

        if (!dontRespond)
        {
            // Send unsubscription confirmation to client
            std::vector<std::string> respArray = {RESPEncoder::encodeString("unsubscribe"),
                RESPEncoder::encodeString(sub->GetChannelName()),
                RESPEncoder::encodeInteger(--channelsSubscribed)};

            std::string response = RESPEncoder::encodeArray(respArray, true);
            send(clientFd, response.c_str(), response.length(), 0);
        }

        std::cout << "Client " << clientFd << " unsubscribed from channel " << sub->GetChannelName() << std::endl;
    }

    m_clientSubscriptions.erase(clientFd);
    std::cout << "Client " << clientFd << " has no more subscriptions!" << std::endl;
}

std::string SubscriptionHandler::publishHandler(CommandArray commandArgs)
{
    if (commandArgs->size() != 3)
    {
        return RESPEncoder::encodeError("wrong number of arguments for 'publish' command");
    }

    std::string &channelName = (*commandArgs)[1];
    std::string &message = (*commandArgs)[2];

    if (m_channelSubscriptions.find(channelName) == m_channelSubscriptions.end())
    {
        return RESPEncoder::encodeInteger(0); // No subscribers
    }

    auto &sub = m_channelSubscriptions[channelName];
    int receivers = 0;

    for (int clientFd : sub->GetSubscribedClients())
    {
        std::vector<std::string> respArray = {RESPEncoder::encodeString("message"),
            RESPEncoder::encodeString(channelName),
            RESPEncoder::encodeString(message)};

        std::string response = RESPEncoder::encodeArray(respArray, true);
        send(clientFd, response.c_str(), response.length(), 0);
        receivers++;
    }

    std::cout << "Published message to channel " << channelName << " for " << receivers << " subscribers." << std::endl;

    return RESPEncoder::encodeInteger(receivers);
}

SubscriptionHandler::~SubscriptionHandler()
{
    // std::cout << "Channels present at destruction: " << m_channelSubscriptions.size() << std::endl;
    // std::cout << "Clients present at destruction: " << m_clientSubscriptions.size() << std::endl;

    m_channelSubscriptions.clear();
    m_clientSubscriptions.clear();
}

