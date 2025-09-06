
#include "SupportedCommands.h"
#include "ListHandler.h"
#include "Utility.h"
#include "RESPEncoder.h"
#include <thread>
#include <iostream>
#include <sys/socket.h>

std::string ListHandler::ListCommandProcessor(CommandArray commandArgs, const int clientFd)
{
    if (!commandArgs || commandArgs->empty())
        throw std::runtime_error("Invalid command Array");

    commandArgs->at(0).assign(toLower(commandArgs->at(0)));

    std::string &command = (*commandArgs)[0];
    std::string &listName = (*commandArgs)[1];

    if (command == "lpush")
    {
        return lpushHandler(std::move(commandArgs));
    }
    else if (command == "rpush")
    {
        return rpushHandler(std::move(commandArgs));
    }
    else if (command == "lpop")
    {
        return lpopHandler(std::move(commandArgs));
    }
    else if (command == "rpop")
    {
        // Implement RPOP logic here
        return "$value\r\n"; // Placeholder response
    }
    else if (command == "lrange")
    {
        return lrangeHandler(std::move(commandArgs));
    }
    else if (command == "llen")
    {
        return RESPEncoder::encodeInteger(m_lists.find(listName) != m_lists.end() ? m_lists[listName]->GetListLength() : 0);
    }
    else if (command == "blpop")
    {
        return blpopHandler(std::move(commandArgs), clientFd);
    }

    return RESPEncoder::encodeError("Unsupported list command");
}

std::string ListHandler::lpushHandler(CommandArray commandArgs)
{
    std::string &listName = (*commandArgs)[1];
    std::vector<std::string> elementsToAdd(commandArgs->begin() + 2, commandArgs->end());

    // Remove quotes if present for each element
    std::for_each(elementsToAdd.begin(), elementsToAdd.end(), [](std::string &elem)
    { 
        if (elem.front() == '"' && elem.back() == '"')
            elem = elem.substr(1, elem.size() - 2);
    });

    if (m_lists.find(listName) == m_lists.end())
    {
        m_lists[listName] = std::make_unique<List>(listName);
    }

    std::string result;
    {
        std::lock_guard<std::mutex> lock(m_listsMutex);
        result = m_lists[listName]->AddElementsAtFront(std::move(elementsToAdd));
    }
    setEventForBlockingLists(listName, elementsToAdd.size());

    return std::move(result);
}

std::string ListHandler::rpushHandler(CommandArray commandArgs)
{
    std::string &listName = (*commandArgs)[1];
    std::vector<std::string> elementsToAdd(commandArgs->begin() + 2, commandArgs->end());

    // Remove quotes if present for each element
    std::for_each(elementsToAdd.begin(), elementsToAdd.end(), [](std::string &elem)
    { 
        if (elem.front() == '"' && elem.back() == '"')
            elem = elem.substr(1, elem.size() - 2);
    });

    if (m_lists.find(listName) == m_lists.end())
    {
        m_lists[listName] = std::make_unique<List>(listName);
    }

    std::string result;
    {
        std::lock_guard<std::mutex> lock(m_listsMutex);
        result = m_lists[listName]->AddElementsAtEnd(std::move(elementsToAdd));
    }
    setEventForBlockingLists(listName, elementsToAdd.size());

    return std::move(result);
}

std::string ListHandler::lrangeHandler(CommandArray commandArgs)
{
    std::lock_guard<std::mutex> lock(m_listsMutex);

    if (commandArgs->size() != 4)
    {
        return RESPEncoder::encodeError("wrong number of arguments for 'lrange' command");
    }

    std::string &listName = (*commandArgs)[1];
    std::vector<std::string> results;

    if (m_lists.find(listName) != m_lists.end() && m_lists[listName]->GetListLength() > 0)
    {
        int start = std::stoi((*commandArgs)[2]);
        int end = std::stoi((*commandArgs)[3]);

        if (start < -1 * m_lists[listName]->GetListLength())
            start = 0;
        if (end < -1 * m_lists[listName]->GetListLength())
            end = 0;

        if (start >= m_lists[listName]->GetListLength())
            start = m_lists[listName]->GetListLength() - 1;
        if (end >= m_lists[listName]->GetListLength())
            end = m_lists[listName]->GetListLength() - 1;

        start = (start + m_lists[listName]->GetListLength()) % m_lists[listName]->GetListLength();
        end = (end + m_lists[listName]->GetListLength()) % m_lists[listName]->GetListLength();

        if (m_lists.find(listName) != m_lists.end())
        {
            auto &lst = m_lists[listName];
            auto it = lst->m_listStore.begin();
            std::advance(it, start);

            for (int i = start; i <= end && it != lst->m_listStore.end(); ++i, ++it)
            {
                results.push_back(*it);
            }
        }
    }

    return RESPEncoder::encodeArray(results);
}

std::string ListHandler::lpopHandler(CommandArray commandArgs)
{
    std::lock_guard<std::mutex> lock(m_listsMutex); // As this func is called from blocking thread too

    std::string &listName = (*commandArgs)[1];
    int itemsToRemove = 1;

    if (commandArgs->size() > 2)
        itemsToRemove = std::stoi((*commandArgs)[2]);

    if (m_lists.find(listName) == m_lists.end() || m_lists[listName]->GetListLength() == 0)
    {
        return "$-1\r\n"; // return nil if list doesn't exist or is empty
    }

    auto &lst = m_lists[listName];
    std::vector<std::string> removedElements;

    for (int i = 0; i < itemsToRemove && !lst->m_listStore.empty(); ++i)
    {
        removedElements.push_back(lst->m_listStore.front());
        lst->m_listStore.pop_front();
    }

    return removedElements.size() > 1 ? RESPEncoder::encodeArray(removedElements) : RESPEncoder::encodeString(removedElements.front());
}

std::string ListHandler::rpopHandler(CommandArray commandArgs)
{
    std::lock_guard<std::mutex> lock(m_listsMutex);

    std::string &listName = (*commandArgs)[1];
    int itemsToRemove = 1;

    if (commandArgs->size() > 2)
        itemsToRemove = std::stoi((*commandArgs)[2]);

    if (m_lists.find(listName) == m_lists.end() || m_lists[listName]->GetListLength() == 0)
    {
        return "$-1\r\n"; // return nil if list doesn't exist or is empty
    }

    auto &lst = m_lists[listName];
    std::vector<std::string> removedElements;

    for (int i = 0; i < itemsToRemove && !lst->m_listStore.empty(); ++i)
    {
        removedElements.push_back(lst->m_listStore.back());
        lst->m_listStore.pop_back();
    }

    return removedElements.size() > 1 ? RESPEncoder::encodeArray(removedElements) : RESPEncoder::encodeString(removedElements.front());
}

std::string ListHandler::blpopHandler(CommandArray commandArgs, const int clientFd)
{
    if (commandArgs->size() < 3)
        return RESPEncoder::encodeError("wrong number of arguments for 'blpop' command");

    std::vector<std::string> listNames;

    for (auto it = commandArgs->begin() + 1; it != commandArgs->end() - 1; ++it)
    {
        listNames.push_back(*it);
        
        // Check if any list has elements to pop immediately
        std::vector<std::string> tempArgs = {LPOP, *it};
        auto result = lpopHandler(std::make_unique<std::vector<std::string>>(tempArgs));
        if (result != NULL_BULK_ENCODED) // Found an element
        {
            // Format response as per BLPOP requirements
            std::vector<std::string> respArray = {RESPEncoder::encodeString(*it), result};
            return RESPEncoder::encodeArray(respArray, true);
        }
    }

    std::string blockingVal = commandArgs->back();
    if (blockingVal == "0")
        blockingVal = std::to_string(10 * 60); // default to 10 minutes if 0 is specified

    m_blockingLists[clientFd] = {listNames, std::make_shared<EventWaiter>()};

    std::thread blockingThread([this, blockingVal, clientFd]()
    {
        bool eventOccurred = m_blockingLists[clientFd].second->waitForEvent(std::chrono::milliseconds(std::stoul(blockingVal) * 1000));
        std::string response;

        if (eventOccurred)
        {
            // Check each list for available elements
            for (const auto& listName : m_blockingLists[clientFd].first)
            {
                std::vector<std::string> commandArgs = {LPOP, listName};
                response = lpopHandler(std::make_unique<std::vector<std::string>>(commandArgs));
                if (response != NULL_BULK_ENCODED) // Found an element
                {
                    // Format response as per BLPOP requirements
                    std::vector<std::string> respArray = {RESPEncoder::encodeString(listName), response};
                    response = RESPEncoder::encodeArray(respArray, true);
                    break;
                }
            }
        }
        if (response.empty())
        {
            response = "*-1\r\n"; // Timeout or no element found, return nil array
        }

        {
            std::lock_guard<std::mutex> lock(m_blockingListsMutex);
            m_blockingLists.erase(clientFd);
        }

        std::cout << "Sending response from blocking thread..." << response << std::endl;
        send(clientFd, response.c_str(), response.length(), 0);
    });

    blockingThread.detach();
    return NO_REPLY; // Indicate that response will be sent later
}

void ListHandler::setEventForBlockingLists(const std::string &listName, int noOfElementsAdded)
{
    std::lock_guard<std::mutex> lock(m_blockingListsMutex);
    std::list<std::shared_ptr<EventWaiter>> waitersToNotify;

    for (auto& [clientFd, pair] : m_blockingLists)
    {
        auto& [listNames, eventWaiter] = pair;
        if (std::find(listNames.begin(), listNames.end(), listName) != listNames.end())
        {
            waitersToNotify.push_back(eventWaiter);
        }
    }

    for (int i = 0; i < noOfElementsAdded && !waitersToNotify.empty(); ++i)
    {
        auto eventWaiter = waitersToNotify.front();
        waitersToNotify.pop_front();

        eventWaiter->setEvent();
    }
}


std::string List::AddElementsAtEnd(const std::vector<std::string> &elementsToAdd)
{
    for (const auto &element : elementsToAdd)
        m_listStore.push_back(element);

    return RESPEncoder::encodeInteger(m_listStore.size()); // return new length of the list
}

std::string List::AddElementsAtFront(const std::vector<std::string> &elementsToAdd)
{
    for (const auto &element : elementsToAdd)
        m_listStore.push_front(element);

    return RESPEncoder::encodeInteger(m_listStore.size()); // return new length of the list
}
