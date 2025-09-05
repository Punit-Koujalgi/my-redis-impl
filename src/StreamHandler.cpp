#include <stdexcept>
#include <chrono>
#include <iostream>

#include "StreamHandler.h"
#include "RESPEncoder.h"
#include "Utility.h"
#include "SupportedCommands.h"

std::tuple<unsigned long, unsigned long> StreamHandler::parseEntryId(const std::string &streamName, const std::string &entryId)
{
    // strip quotes
    std::string unquotedEntryId = entryId;
    if (unquotedEntryId.front() == '"' && unquotedEntryId.back() == '"')
    {
        unquotedEntryId = unquotedEntryId.substr(1, unquotedEntryId.size() - 2);
    }

    auto separatorPos = unquotedEntryId.find('-');

    unsigned long firstId{}, secondId{};

    if (unquotedEntryId.substr(0, separatorPos) == "*")
    {
        m_streams[streamName]->setFirstIdDefault();
    }
    else
    {
        firstId = std::stoul(unquotedEntryId.substr(0, separatorPos));

        if (unquotedEntryId.substr(separatorPos + 1) == "*")
        {
            m_streams[streamName]->setSecondIdDefault();
        }
        else
        {
            secondId = std::stoul(unquotedEntryId.substr(separatorPos + 1));
        }
    }

    return std::make_tuple(firstId, secondId);
}

bool StreamHandler::IsStreamPresent(const std::string& name)
{
    return m_streams.find(name) != m_streams.end();
}

// stream processor
std::string StreamHandler::StreamCommandProcessor(CommandArray commandArgs)
{
    if (!commandArgs || commandArgs->empty())
        throw std::runtime_error("Invalid command Array");

    commandArgs->at(0).assign(toLower(commandArgs->at(0)));

    std::string_view command = (*commandArgs)[0];
    if (command == XADD)
    {
        return xaddHandler(std::move(commandArgs));
    }
    else if (command == XRANGE)
    {
        // Handle Querying a stream using xrange
        return xrangeHandler(std::move(commandArgs));
    }

    throw std::runtime_error("Invalid Command!");
}

std::string StreamHandler::xaddHandler(CommandArray commandArgs)
{
    std::cout << "Processing xadd.." << std::endl; 
    // validate commandArray for xadd
    if (commandArgs->size() < 4 || (commandArgs->size() - 3) % 2 != 0)
    {
        return RESPEncoder::encodeError("wrong number of arguments for 'xadd' command");
    }

    // Handle adding a new stream if not present or add entry to existing stream
    std::string &streamName = (*commandArgs)[1];
    if (m_streams.find(streamName) == m_streams.end())
    {
        // Create a new stream
        m_streams[streamName] = std::make_unique<Stream>(streamName);
    }

    std::string &entryId = (*commandArgs)[2];
    auto [firstId, secondId] = parseEntryId(streamName, entryId);

    std::map<std::string, std::string> fieldValues;
    for (auto it = commandArgs->begin() + 3; it != commandArgs->end(); ++it)
    {
        std::string &field = *it;
        std::string &value = *(++it);
        fieldValues[field] = value;
    }

    // Add entry to the stream
    return m_streams[streamName]->AddEntry(firstId, secondId, fieldValues);
}

std::string StreamHandler::xrangeHandler(CommandArray commandArgs)
{
    if (commandArgs->size() < 4)
    {
        return RESPEncoder::encodeError("wrong number of arguments for 'xrange' command");
    }

    std::string &streamName = (*commandArgs)[1];
    if (m_streams.find(streamName) == m_streams.end())
    {
        // return empty array
        return RESPEncoder::encodeArray({});
    }
    else
    {
        // Handle querying the stream for entries in the given range
        std::string &startId = (*commandArgs)[2];
        std::string &endId = (*commandArgs)[3];

        auto entries = m_streams[streamName]->GetEntriesInRange(startId, endId);
        std::vector<std::string> mainReturnArray;

        // Form result and format into RESP
        for (const auto& entry : entries)
        {
            // Format each entry into RESP
            std::vector<std::string> entryArray, keyvalueArray;
            entryArray.push_back(RESPEncoder::encodeString(entry.first)); // Entry ID

            for (const auto& kv : entry.second)
            {
                keyvalueArray.push_back(RESPEncoder::encodeString(kv.first));
                keyvalueArray.push_back(RESPEncoder::encodeString(kv.second));
            }
            entryArray.push_back(RESPEncoder::encodeArray(keyvalueArray, true /* dontEncodeItems */));
            mainReturnArray.push_back(RESPEncoder::encodeArray(entryArray, true));
        }
        
        return RESPEncoder::encodeArray(mainReturnArray, true);
    }
}

std::string Stream::AddEntry(unsigned long entryFirstId, unsigned long entrySecondId, const std::map<std::string, std::string> &fieldValues)
{
    std::cout << "Adding entry with Id: " << entryFirstId << "-" << entrySecondId << std::endl;

    // Handle default cases
    if (firstIdDefault)
    {
        // Auto-generate timestamp
        auto now = std::chrono::system_clock::now();
        entryFirstId = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        entrySecondId = 0;
    }
    else if (secondIdDefault)
    {
        if (m_streamStore.find(entryFirstId) != m_streamStore.end())
        {
            // Find the highest sequence number for the given millisecond part
            auto &secondIdMap = m_streamStore[entryFirstId];
            entrySecondId = secondIdMap.rbegin()->first + 1; // Increment the highest sequence number
        }
        else
        {
            entrySecondId = entryFirstId == 0 ? 1 : 0; // First entry for this millisecond part
        }
    }

    // std::cout << "latest Ids:" << m_latestFirstId << "-" << m_latestSecondId << std::endl;

    if (entryFirstId == 0 && entrySecondId == 0)
    {
        return RESPEncoder::encodeError("The ID specified in XADD must be greater than 0-0");
    }

    if (entryFirstId < m_latestFirstId ||
        (entryFirstId == m_latestFirstId && entrySecondId <= m_latestSecondId))
    {
        return RESPEncoder::encodeError("The ID specified in XADD is equal or smaller than the target stream top item");
    }

    // Update the latest IDs
    m_latestFirstId = entryFirstId;
    m_latestSecondId = entrySecondId;

    // Add the entry to the stream store
    m_streamStore[entryFirstId][entrySecondId] = std::move(fieldValues);

    return RESPEncoder::encodeString(std::to_string(entryFirstId) + "-" + std::to_string(entrySecondId));
}

void Stream::setFirstIdDefault()
{
    firstIdDefault = true;
}

void Stream::setSecondIdDefault()
{
    secondIdDefault = true;
}

std::tuple<unsigned long, unsigned long, unsigned long, unsigned long> Stream::processRange(const std::string &startId, const std::string &endId)
{
    unsigned long startMilliSecondId{}, startsequenceId{};
    unsigned long endMilliSecondId{}, endsequenceId{};

    if (startId == "-")
    {
        startMilliSecondId = 0;
        startsequenceId = 0;
    }
    else
    {
        auto startSep = startId.find("-");
        startMilliSecondId = std::stoul(startId.substr(0, startSep));
        if (startSep != std::string::npos)
            startsequenceId = std::stoul(startId.substr(startSep + 1));
    }

    if (endId == "+")
    {
        endMilliSecondId = 0;
        endsequenceId = 0;
    }
    else
    {
        auto endSep = endId.find("-");
        endMilliSecondId = std::stoul(endId.substr(0, endSep));
        if (endSep != std::string::npos)
            endsequenceId = std::stoul(endId.substr(endSep + 1));
    }

    return std::make_tuple(startMilliSecondId, startsequenceId, endMilliSecondId, endsequenceId);
}

std::map<std::string, std::map<std::string, std::string>> Stream::GetEntriesInRange(const std::string &startId, const std::string &endId)
{
    auto [startMilliSecondId, startsequenceId, endMilliSecondId, endsequenceId] = processRange(startId, endId);

    // To store result entries
    std::map<std::string, std::map<std::string, std::string>> resultEntries;

    auto firstEntry = startMilliSecondId == 0 ? m_streamStore.begin() : m_streamStore.lower_bound(startMilliSecondId);
    if (firstEntry != m_streamStore.end())
    {
        auto lastEntry = endMilliSecondId == 0 ? m_streamStore.end() : m_streamStore.upper_bound(endMilliSecondId);
                                                                                        // End is inclusive
        // Iterate through the entries until we reach the end of the range
        for (auto it = firstEntry; it != lastEntry; ++it)
        {
            if (it->first == startMilliSecondId && startsequenceId != 0)
            {
                auto startSeqEntry = it->second.lower_bound(startsequenceId);

                for (auto seqIt = startSeqEntry; seqIt != it->second.end(); ++seqIt)
                {
                    std::string entryId = std::to_string(it->first) + "-" + std::to_string(seqIt->first);

                    for (auto &kvPair : seqIt->second)
                        resultEntries[entryId][kvPair.first] = kvPair.second;
                }
                continue;
            }

            if (it->first == endMilliSecondId && endsequenceId != 0)
            {
                auto endSeqEntry = it->second.upper_bound(endsequenceId);
                                                    // End is inclusive

                for (auto seqIt = it->second.begin(); seqIt != endSeqEntry; ++seqIt)
                {
                    std::string entryId = std::to_string(it->first) + "-" + std::to_string(seqIt->first);

                    for (auto &kvPair : seqIt->second)
                        resultEntries[entryId][kvPair.first] = kvPair.second;
                }
                continue;
            }

            for (auto seqIt = it->second.begin(); seqIt != it->second.end(); ++seqIt)
            {
                std::string entryId = std::to_string(it->first) + "-" + std::to_string(seqIt->first);

                for (auto &kvPair : seqIt->second)
                    resultEntries[entryId][kvPair.first] = kvPair.second;
            }
        }
    }

    return std::move(resultEntries);
}
