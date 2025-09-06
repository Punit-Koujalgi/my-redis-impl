
#include "ListHandler.h"
#include "Utility.h"
#include "RESPEncoder.h"

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

    return m_lists[listName]->AddElementsAtFront(std::move(elementsToAdd));
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

    return m_lists[listName]->AddElementsAtEnd(std::move(elementsToAdd));
}

std::string ListHandler::lrangeHandler(CommandArray commandArgs)
{
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
