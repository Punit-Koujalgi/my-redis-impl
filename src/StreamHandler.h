#ifndef STREAMHANDLER_H
#define STREAMHANDLER_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

#include "Utility.h"
#include "Stream.h"

using CommandArray = std::unique_ptr<std::vector<std::string>>;

class StreamHandler
{
private:
    std::unordered_map<std::string, std::unique_ptr<Stream>> m_streams;
    std::mutex m_blockingStreamsMutex;
    std::unordered_map<std::string, std::pair<std::string, std::shared_ptr<EventWaiter>>> m_blockingStreams; /* streamName, pair(streamId, EventWaiter) */

    std::tuple<unsigned long, unsigned long> parseEntryId(const std::string& streamName, const std::string &entryId);
    std::string xaddHandler(CommandArray commandArgs);
    std::string xrangeHandler(CommandArray commandArgs);
    
    std::tuple<std::vector<std::string>, std::vector<std::string>, std::string> parseXreadOptions(CommandArray& commandArgs);
    std::string xreadHandler(CommandArray commandArgs, const int clientFd);
    void processBlockingRead(const std::string& blockingVal, const int clientFd, std::vector<std::string> streamNames, std::vector<std::string> streamStartIds);

public:
    bool IsStreamPresent(const std::string& name);
    std::string StreamCommandProcessor(CommandArray commandArgs, const int clientFd);
};

#endif // STREAMHANDLER_H
