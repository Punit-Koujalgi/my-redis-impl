#ifndef STREAMHANDLER_H
#define STREAMHANDLER_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

using CommandArray = std::unique_ptr<std::vector<std::string>>;

class Stream
{
private:
    std::string m_streamName;
    unsigned long m_latestFirstId{};  // usually the millisecond part of the Id
    unsigned long m_latestSecondId{}; // usually the sequence number part of the Id
    bool firstIdDefault{false};
    bool secondIdDefault{false};

    /* Stream Store
       Storing this way to allow for efficient retrieval of stream data by Ids and using binary search
       To extract entries: First we do binary search on millisecondId, then keep including all entries until
       last millisecondId is hit then do binary search on sequenceId and include relevant entries
    */
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
    std::map<std::string, std::map<std::string, std::string>> GetEntriesInRange(const std::string& startId, const std::string& endId);
};

class StreamHandler
{
private:
    std::unordered_map<std::string, std::unique_ptr<Stream>> m_streams;

    std::tuple<unsigned long, unsigned long> parseEntryId(const std::string& streamName, const std::string &entryId);
    std::string xaddHandler(CommandArray commandArgs);
    std::string xrangeHandler(CommandArray commandArgs);

public:
    bool IsStreamPresent(const std::string& name);
    std::string StreamCommandProcessor(CommandArray commandArgs);
};

#endif // STREAMHANDLER_H
