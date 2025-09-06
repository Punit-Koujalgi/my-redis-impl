
#ifndef LIST_HANDLER_H
#define LIST_HANDLER_H

#include <memory>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>

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
    std::unordered_map<std::string, std::unique_ptr<List>> m_lists;

    std::string lpushHandler(CommandArray commandArgs);
    std::string rpushHandler(CommandArray commandArgs);
    std::string lrangeHandler(CommandArray commandArgs);
    std::string lpopHandler(CommandArray commandArgs);
    std::string rpopHandler(CommandArray commandArgs);
};

#endif // LIST_HANDLER_H
