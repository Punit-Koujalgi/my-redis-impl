
#include <iostream>

#include "Stream.h"
#include "RESPEncoder.h"
#include "Utility.h"
#include "SupportedCommands.h"

std::string Stream::AddEntry(unsigned long entryFirstId, unsigned long entrySecondId, const std::map<std::string, std::string> &fieldValues)
{
    std::cout << "Adding entry with Id: " << entryFirstId << "-" << entrySecondId << std::endl;
    std::lock_guard<std::mutex> lock(m_streamStoreMutex);

    // Handle default cases
    if (m_firstIdDefault)
    {
        // Auto-generate timestamp
        auto now = std::chrono::system_clock::now();
        entryFirstId = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        entrySecondId = 0;
    }
    else if (m_secondIdDefault)
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

    // Reset default flags
    m_firstIdDefault = false;
    m_secondIdDefault = false;

    return RESPEncoder::encodeString(std::to_string(entryFirstId) + "-" + std::to_string(entrySecondId));
}

void Stream::setFirstIdDefault()
{
    m_firstIdDefault = true;
}

void Stream::setSecondIdDefault()
{
    m_secondIdDefault = true;
}

std::string Stream::getLatestEntryId() const
{
    return std::move(std::to_string(m_latestFirstId) + "-" + std::to_string(m_latestSecondId));
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

std::map<std::string, std::map<std::string, std::string>> Stream::GetEntriesInRange(const std::string &startId, const std::string &endId, bool exclusiveStart)
{
    auto [startMilliSecondId, startsequenceId, endMilliSecondId, endsequenceId] = processRange(startId, endId);

    // To store result entries
    std::map<std::string, std::map<std::string, std::string>> resultEntries;
    std::lock_guard<std::mutex> lock(m_streamStoreMutex);

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
                if (exclusiveStart && startSeqEntry != it->second.end() && startSeqEntry->first == startsequenceId)
                    ++startSeqEntry; // Move to next entry if exclusive

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
                if (exclusiveStart && it->first == startMilliSecondId && seqIt->first == startsequenceId)
                    continue;

                std::string entryId = std::to_string(it->first) + "-" + std::to_string(seqIt->first);

                for (auto &kvPair : seqIt->second)
                    resultEntries[entryId][kvPair.first] = kvPair.second;
            }
        }
    }

    return std::move(resultEntries);
}


