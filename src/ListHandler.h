
#ifndef LIST_HANDLER_H
#define LIST_HANDLER_H

#include <memory>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <map>
#include <mutex>

class EventWaiter;

using CommandArray = std::unique_ptr<std::vector<std::string>>;

class List
{
public:
    List(const std::string &listName)
        : m_listName(listName) {}

    std::string AddElementsAtEnd(const std::vector<std::string> &elementsToAdd);
    std::string AddElementsAtFront(const std::vector<std::string> &elementsToAdd);
    int GetListLength() const { return m_listStore.size(); }

private:
    std::string m_listName;
    std::list<std::string> m_listStore;

    friend class ListHandler;
};

class ListHandler
{
public:
    std::string ListCommandProcessor(CommandArray commandArgs, const int clientFd);

private:
    std::mutex m_listsMutex;
    std::unordered_map<std::string, std::unique_ptr<List>> m_lists;
    
    std::mutex m_blockingListsMutex;
    std::map<int, std::pair<std::vector<std::string>, std::shared_ptr<EventWaiter>>> m_blockingLists; /* listName, pair(vector<listNames>, EventWaiter) */

    std::string lpushHandler(CommandArray commandArgs);
    std::string rpushHandler(CommandArray commandArgs);
    std::string lrangeHandler(CommandArray commandArgs);
    std::string lpopHandler(CommandArray commandArgs);
    std::string rpopHandler(CommandArray commandArgs);
    
    std::string blpopHandler(CommandArray commandArgs, const int clientFd);
    void setEventForBlockingLists(const std::string &listName, int noOfElementsAdded);
};

#endif // LIST_HANDLER_H
