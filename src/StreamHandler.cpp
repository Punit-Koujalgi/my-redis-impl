#include <stdexcept>
#include <chrono>
#include <iostream>
#include <ranges>
#include <future>
#include <sys/socket.h>

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
            secondId = separatorPos == std::string::npos ? 0 : std::stoul(unquotedEntryId.substr(separatorPos + 1));
        }
    }

    return std::make_tuple(firstId, secondId);
}

bool StreamHandler::IsStreamPresent(const std::string &name)
{
    return m_streams.find(name) != m_streams.end();
}

// stream processor
std::string StreamHandler::StreamCommandProcessor(CommandArray commandArgs, const int clientFd)
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
    else if (command == XREAD)
    {
        return xreadHandler(std::move(commandArgs), clientFd);
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
    auto result = m_streams[streamName]->AddEntry(firstId, secondId, fieldValues);

    {
        std::lock_guard<std::mutex> lock(m_blockingStreamsMutex);
        if (m_blockingStreams.find(streamName) != m_blockingStreams.end())
        {
            auto &[waitId, eventWaiter] = m_blockingStreams[streamName];
            auto [waitFirstId, waitSecondId] = parseEntryId(streamName, waitId);

            std::cout << firstId << "-" << secondId << " vs WaitId: " << waitFirstId << "-" << waitSecondId << std::endl;
            // If the added entry is greater than the wait Id, signal the waiter
            if (firstId > waitFirstId || (firstId == waitFirstId && secondId > waitSecondId))
            {
                std::cout << "Signaling waiter for stream: " << streamName << std::endl;
                eventWaiter->setEvent();
            }
        }
    }

    return result;
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
        for (const auto &entry : entries)
        {
            // Format each entry into RESP
            std::vector<std::string> entryArray, keyvalueArray;
            entryArray.push_back(RESPEncoder::encodeString(entry.first)); // Entry ID

            for (const auto &kv : entry.second)
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

std::tuple<std::vector<std::string>, std::vector<std::string>, std::string> StreamHandler::parseXreadOptions(CommandArray &commandArgs)
{
    std::vector<std::string> streamNames;
    std::vector<std::string> streamStartIds;

    int count = 1;
    bool readBlocking = false;
    std::string blockingVal;

    for (const auto &arg : *commandArgs)
    {
        if (toLower(arg) == XREAD || toLower(arg) == STREAMS)
        {
            ++count;
            continue;
        }

        if (toLower(arg) == "block")
        {
            ++count;
            readBlocking = true;
            continue;
        }

        if (readBlocking && blockingVal.empty())
        {
            blockingVal = arg;
            ++count;
            continue;
        }

        if (count > (commandArgs->size() / 2 + (readBlocking ? 2 : 1))) // Ex: XREAD block 1000 streams stream_key other_stream_key 0-0 0-1
            streamStartIds.push_back(arg);
        else
            streamNames.push_back(arg);

        ++count;
    }

    std::cout << "Streams to read: " << streamNames.size() << ", Start Ids: " << streamStartIds.size() << std::endl;

    return {std::move(streamNames), std::move(streamStartIds), std::move(blockingVal)};
}

std::string StreamHandler::xreadHandler(CommandArray commandArgs, const int clientFd)
{
    // validate command
    if (commandArgs->size() < 4 || commandArgs->size() % 2 != 0)
        return RESPEncoder::encodeError("wrong number of arguments for 'xread' command");

    auto [streamNames, streamStartIds, blockingVal] = parseXreadOptions(commandArgs);
    std::vector<std::string> mainReturnArray;

    // Process the streams
    for (size_t i = 0; i < streamNames.size(); ++i)
    {
        const auto &streamName = streamNames[i];
        auto &streamStartId = streamStartIds[i];

        std::cout << "Processing stream: " << streamName << " from Id: " << streamStartId << std::endl;

        if (streamStartId == "$")
            streamStartId = m_streams[streamName]->getLatestEntryId();

        // Get the entries for the stream
        auto entries = m_streams[streamName]->GetEntriesInRange(streamStartId, "+", true /* exclusiveStart */);
        if (entries.empty())
            continue;

        std::vector<std::string> streamArray;
        std::vector<std::string> entriesArray;

        streamArray.push_back(RESPEncoder::encodeString(streamName));

        for (const auto &entry : entries)
        {
            // Format each entry into RESP
            std::vector<std::string> entryArray, keyvalueArray;
            entryArray.push_back(RESPEncoder::encodeString(entry.first)); // Entry ID

            for (const auto &kv : entry.second)
            {
                keyvalueArray.push_back(RESPEncoder::encodeString(kv.first));
                keyvalueArray.push_back(RESPEncoder::encodeString(kv.second));
            }
            entryArray.push_back(RESPEncoder::encodeArray(keyvalueArray, true /* dontEncodeItems */));
            entriesArray.push_back(RESPEncoder::encodeArray(entryArray, true));
        }

        streamArray.push_back(RESPEncoder::encodeArray(entriesArray, true));
        mainReturnArray.push_back(RESPEncoder::encodeArray(streamArray, true));
    }

    if (mainReturnArray.empty() && !blockingVal.empty())
    {
        processBlockingRead(blockingVal, clientFd, std::move(streamNames), std::move(streamStartIds));
        return NO_REPLY;
    }

    if (mainReturnArray.empty())
        return "*-1\r\n"; // return nil array if no entries found

    return RESPEncoder::encodeArray(mainReturnArray, true);
}

void StreamHandler::processBlockingRead(const std::string &blockingVal, const int clientFd, std::vector<std::string> streamNames, std::vector<std::string> streamStartIds)
{
    std::cout << "Going to start Blocking Read..." << std::endl;

    std::atomic<bool> shouldStop = false;
    std::vector<std::thread> threads;

    auto sharedEvent = std::make_shared<EventWaiter>();

    for (const auto &[index, streamName] : streamNames | std::views::enumerate)
    {
        const auto &streamStartId = streamStartIds[index];
        std::string waitEntryId = streamStartId;

        if (streamStartId == "$")
            waitEntryId = m_streams[streamName]->getLatestEntryId();

        {
            std::lock_guard<std::mutex> lock(m_blockingStreamsMutex);

            m_blockingStreams[streamName].first = waitEntryId;
            m_blockingStreams[streamName].second = sharedEvent;
        }

    }

    std::string blockingValLocal = blockingVal; 
    if (blockingVal == "0")
        blockingValLocal = std::to_string(10 * 60 * 1000); // Default to 10 minutes, max wait time supported

    std::thread blockingThread([this, blockingValLocal, clientFd, sharedEvent, streamNames, streamStartIds]()
    {
        bool eventOccurred = sharedEvent->waitForEvent(std::chrono::milliseconds(std::stoul(blockingValLocal)));
        std::string response;

        if (eventOccurred)
        {
            // Form new xread command to fetch new entries
            std::vector<std::string> commandArgs = {XREAD, STREAMS};
            for (const auto& streamName : streamNames)
            {
                commandArgs.push_back(streamName);
            }
            for (const auto& streamId : streamStartIds)
            {
                commandArgs.push_back(streamId);
            }

            response = xreadHandler(std::make_unique<std::vector<std::string>>(commandArgs), clientFd);
        }
        else
        {
            response = "*-1\r\n"; // Timeout, return nil array
        }

        {
            std::lock_guard<std::mutex> lock(m_blockingStreamsMutex);
            m_blockingStreams.clear();
        }

        std::cout << "Sending response from blocking thread..." << response << std::endl;
        send(clientFd, response.c_str(), response.length(), 0);
    });

    blockingThread.detach();

    //     threads.emplace_back([this, streamName, blockingVal, clientFd, &shouldStop]()
    //     {
    //         bool eventOccurred = false;

    //         if (blockingVal == "0")
    //         {
    //             // Wait indefinitely but check for stop signal
    //             while (!shouldStop.load() && !eventOccurred)
    //             {
    //                 std::lock_guard<std::mutex> lock(m_blockingStreamsMutex);

    //                 eventOccurred = m_blockingStreams[streamName].second->waitForEvent(
    //                     std::chrono::milliseconds(100)); // Short timeout to check stop flag
    //             }

    //         }
    //         else
    //         {
    //             auto timeout = std::chrono::milliseconds(std::stoul(blockingVal));
    //             auto deadline = std::chrono::steady_clock::now() + timeout;
                
    //             while (!shouldStop.load() && std::chrono::steady_clock::now() < deadline && !eventOccurred)
    //             {
    //                 std::lock_guard<std::mutex> lock(m_blockingStreamsMutex);

    //                 auto remaining = deadline - std::chrono::steady_clock::now();
    //                 auto waitTime = std::min(std::chrono::duration_cast<std::chrono::milliseconds>(remaining), std::chrono::milliseconds(100));
                    
    //                 eventOccurred = m_blockingStreams[streamName].second->waitForEvent(waitTime);
    //             }
    //         }

    //         if (!shouldStop.load())
    //         {
    //             // Use compare_and_swap to ensure only one thread responds
    //             bool expected = false;
    //             if (shouldStop.compare_exchange_strong(expected, true))
    //             {
    //                 std::string response = xreadHandler(std::make_unique<std::vector<std::string>>(
    //                     std::vector<std::string>{XREAD, STREAMS, streamName, m_blockingStreams[streamName].first}),
    //                     clientFd);

    //                 std::cout << "Sending response from blocking thread..." << response << std::endl;
    //                 send(clientFd, response.c_str(), response.length(), 0);
    //             }

    //         }

    //         // Clean up
    //         {
    //             std::lock_guard<std::mutex> lock(m_blockingStreamsMutex);
    //             m_blockingStreams.erase(streamName);
    //         }
    //     });

    // }

    // for (auto &t : threads)
    // {
    //     t.detach();
    // }
}

