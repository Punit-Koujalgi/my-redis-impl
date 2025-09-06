#ifndef STREAMHANDLER_H
#define STREAMHANDLER_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

#include "Utility.h"

using CommandArray = std::unique_ptr<std::vector<std::string>>;

class Stream
{
private:
    std::string m_streamName;
    unsigned long m_latestFirstId{};  // usually the millisecond part of the Id
    unsigned long m_latestSecondId{}; // usually the sequence number part of the Id
    bool m_firstIdDefault{false};
    bool m_secondIdDefault{false};

    /* Stream Store
       Storing this way to allow for efficient retrieval of stream data by Ids and using binary search
       To extract entries: First we do binary search on millisecondId, then keep including all entries until
       last millisecondId is hit then do binary search on sequenceId and include relevant entries
    */
    std::mutex m_streamStoreMutex;
    std::map<unsigned long,
             std::map<unsigned long,
                      std::map<std::string, std::string>>>
        m_streamStore;

        std::tuple<unsigned long, unsigned long, unsigned long, unsigned long> processRange(const std::string& startId, const std::string& endId);
public:
    Stream(const std::string &streamName)
        : m_streamName(streamName) {}

    std::string AddEntry(unsigned long entryFirstId, unsigned long entrySecondId, const std::map<std::string, std::string> &fieldValues);
    void setFirstIdDefault();
    void setSecondIdDefault();
    std::string getLatestEntryId() const;
    std::map<std::string, std::map<std::string, std::string>> GetEntriesInRange(const std::string& startId, const std::string& endId, bool exclusiveStart = false);
};

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
