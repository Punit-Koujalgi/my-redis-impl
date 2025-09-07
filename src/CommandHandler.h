
#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <memory>
#include <vector>
#include <string>

#include "Utility.h"
#include "SupportedCommands.h"
#include "KeyValueStore.h"

class Server;
class StreamHandler;
using CommandArray = std::unique_ptr<std::vector<std::string>>;

class CommandHandler
{
public:

    static std::string PING_cmdHandler(CommandArray commandArgs);
    static std::string ECHO_cmdHandler(CommandArray commandArgs);
    static std::string COMMAND_cmdHandler(CommandArray commandArgs);
    static std::string SET_cmdHandler(CommandArray commandArgs, KeyValueStore& kvStore);
    static std::string GET_cmdHandler(CommandArray commandArgs, KeyValueStore& kvStore);
    static std::string CONFIG_cmdHandler(CommandArray commandArgs, std::unordered_map<std::string, std::string>& mapConfiguration);
    //static std::string SAVE_cmdHandler(CommandArray commandArgs);
    static std::string KEYS_cmdHandler(CommandArray commandArgs, KeyValueStore& kvStore);
    static std::string INFO_cmdHandler(CommandArray commandArgs, Server& server);
    static std::string REPLCONF_cmdHandler(CommandArray commandArgs, Server& server, const int clientFd);
    static std::string PSYNC_cmdHandler(CommandArray commandArgs, Server& server, const int clientFd);
    static std::string WAIT_cmdHandler(CommandArray commandArgs, Server& server);
    static std::string TYPE_cmdHandler(CommandArray commandArgs, KeyValueStore& kvStore, StreamHandler& streamHandler);
    static std::string INCR_cmdHandler(CommandArray commandArgs, KeyValueStore& kvStore);
    static std::string TRANSACTION_cmdHandler(CommandArray commandArgs, Server& server, const int clientFd); // MULTI, EXEC, DISCARD
    static std::string SUBSCRIPTION_cmdHandler(CommandArray commandArgs, Server& server, const int clientFd);
};


#endif // COMMANDHANDLER_H

