
#include <iostream>
#include <sys/socket.h>
#include <sys/time.h>

#include "CommandHandler.h"
#include "RESPEncoder.h"
#include "RESPDecoder.h"
#include "Server.h"
#include "SocketReader.h"

std::string CommandHandler::PING_cmdHandler(CommandArray commandArgs)
{
    return "+PONG\r\n";
}

std::string CommandHandler::ECHO_cmdHandler(CommandArray commandArgs)
{
    if (commandArgs->size() != 2)
        return RESPEncoder::encodeError("wrong number of arguments for 'echo' command");

    return "+" + commandArgs->at(1) + "\r\n";
}

std::string CommandHandler::COMMAND_cmdHandler(CommandArray commandArgs)
{
    if (commandArgs->size() != 2)
        return RESPEncoder::encodeError("wrong number of arguments for 'command'");

    return "+OK\r\n";
}

std::string CommandHandler::SET_cmdHandler(CommandArray commandArgs, KeyValueStore &kvStore)
{
    if (commandArgs->size() == 5 && toLower(commandArgs->at(3)) == "px")
    {
        return kvStore.set(commandArgs->at(1), commandArgs->at(2), stoi(commandArgs->at(4)));
    }
    else if (commandArgs->size() == 3)
    {
        return kvStore.set(commandArgs->at(1), commandArgs->at(2));
    }

    return RESPEncoder::encodeError("wrong number of arguments for 'set' command");
}

std::string CommandHandler::GET_cmdHandler(CommandArray commandArgs, KeyValueStore &kvStore)
{
    if (commandArgs->size() != 2)
        return RESPEncoder::encodeError("wrong number of arguments for 'get' command");

    return kvStore.get(commandArgs->at(1));
}

std::string CommandHandler::CONFIG_cmdHandler(CommandArray commandArgs, std::unordered_map<std::string, std::string> &mapConfiguration)
{
    if (commandArgs->size() != 3)
        return RESPEncoder::encodeError("wrong number of arguments for 'config' command");

    if (toLower(commandArgs->at(1)) == "get" &&
        mapConfiguration.find(commandArgs->at(2)) != mapConfiguration.end())
    {
        return RESPEncoder::encodeArray({commandArgs->at(2), mapConfiguration[commandArgs->at(2)]});
    }

    return RESPEncoder::encodeError("Unsupported CONFIG subcommand or invalid parameter");
}

std::string CommandHandler::KEYS_cmdHandler(CommandArray commandArgs, KeyValueStore &kvStore)
{
    if (commandArgs->size() != 2)
        return RESPEncoder::encodeError("wrong number of arguments for 'keys' command");

    return RESPEncoder::encodeArray(*kvStore.getAllKeys(commandArgs->at(1)));
}

std::string CommandHandler::INFO_cmdHandler(CommandArray commandArgs, Server& server)
{
    if (commandArgs->size() != 2)
        return RESPEncoder::encodeError("wrong number of arguments for 'info' command");

    if (toLower(commandArgs->at(1)) == "replication")
    {
        std::string role = server.getReplicationRole();
        std::string result = "role:" + role + "\n";

        if (role == "master")
        {
            result.append("master_replid:");
            result.append(server.m_mapConfiguration["master_replid"]);
            result.append("\n");
            result.append("master_repl_offset:");
            result.append(server.m_mapConfiguration["master_repl_offset"]);
            result.append("\n");
        }

        return RESPEncoder::encodeString(result);
    }

    return RESPEncoder::encodeError("Unsupported INFO section");
}

std::string CommandHandler::REPLCONF_cmdHandler(CommandArray commandArgs, Server &server, const int clientFd)
{
    if (commandArgs->size() == 3 && commandArgs->at(1) == "listening-port")
    {
        server.m_mapReplicaPortSocket[commandArgs->at(2)] = clientFd;
        std::cout << "Got Replica connection [port: " << commandArgs->at(2) << "]" << std::endl;
    }

    if (commandArgs->size() == 3 && toLower(commandArgs->at(1)) == "getack")
    {
        if (toLower(commandArgs->at(1)) == "wait")
            return RESPEncoder::encodeArray({REPLCONF, "ACK", server.m_mapConfiguration["waitcmd_offset"]});
        else
            return RESPEncoder::encodeArray({REPLCONF, "ACK", server.m_mapConfiguration["master_repl_offset"]});
    }

    return "+OK\r\n";
}

std::string CommandHandler::PSYNC_cmdHandler(CommandArray commandArgs, Server &server, const int clientFd)
{
    std::string result = "FULLRESYNC " +
				server.m_mapConfiguration["master_replid"] + " " +
				server.m_mapConfiguration["master_repl_offset"];

		result = RESPEncoder::encodeSimpleString(result);

		std::cout << "Sending response..." << std::endl;
		send(clientFd, result.c_str(), result.length(), 0);

		/* Send initial empty rdb file */
		const std::string empty_rdb = "\x52\x45\x44\x49\x53\x30\x30\x31\x31\xfa\x09\
\x72\x65\x64\x69\x73\x2d\x76\x65\x72\x05\x37\x2e\x32\x2e\x30\xfa\x0a\x72\x65\
\x64\x69\x73\x2d\x62\x69\x74\x73\xc0\x40\xfa\x05\x63\x74\x69\x6d\x65\xc2\x6d\
\x08\xbc\x65\xfa\x08\x75\x73\x65\x64\x2d\x6d\x65\x6d\xc2\xb0\xc4\x10\x00\xfa\
\x08\x61\x6f\x66\x2d\x62\x61\x73\x65\xc0\x00\xff\xf0\x6e\x3b\xfe\xc0\xff\x5a\xa2";

		return "$" + std::to_string(empty_rdb.length()) + "\r\n" + empty_rdb;
}

std::string CommandHandler::WAIT_cmdHandler(CommandArray commandArgs, Server &server)
{
    if (server.m_mapConfiguration["waitcmd_offset"] == "0")
		{
			std::cout << "No write commands yet" << std::endl;
			return RESPEncoder::encodeInteger(server.m_mapReplicaPortSocket.size());
		}

		int replicaThreshold = std::stoi(commandArgs->at(1));
		int timeThreshold = std::stoi(commandArgs->at(2));
		int replicasMetThreshold = 0;

		std::cout << "Got thresholds: " << replicaThreshold << " " << timeThreshold << std::endl;
		// Trying FD approach
		fd_set replSockets, readySockets;
		FD_ZERO(&replSockets);

		for (auto& replica : server.m_mapReplicaPortSocket)
		{
			FD_SET(replica.second, &replSockets);

			std::cout << "Sending req to replica: " << replica.first << " socket: " << replica.second << std::endl;
			// sendData(replica.second, {REPLCONF, "getack", "wait"});
			server.sendData(replica.second, {"REPLCONF", "GETACK", "*"});
		}

		timeval timeUntil;
		gettimeofday(&timeUntil, nullptr);

		// set timeout
		timeUntil.tv_usec += timeThreshold * 1000;
		if (timeUntil.tv_usec > 1000000)
		{// update seconds
			timeUntil.tv_sec += timeUntil.tv_usec / 1000000;
			timeUntil.tv_usec = timeUntil.tv_usec % 1000000;
		}

		while (true)
		{
			timeval timeout;
			timeout.tv_usec = 0;
			timeout.tv_sec = 0; // polling; not right need to change

			readySockets = replSockets;

			if (select(FD_SETSIZE, &readySockets, nullptr, nullptr, &timeout) < 0)
			{
				std::cout << "select() error or timed out";
				break;
			}

			for (int currSock{0}; currSock < FD_SETSIZE; ++currSock)
			{
				if (FD_ISSET(currSock, &readySockets))
				{
					try
					{
						auto ackResponse{SocketReader(currSock).ReadArray()};
						if (ackResponse.size() == 3)
						{
							std::cout << "[Replica: " << currSock << "] is up to date. Offset: " << std::stoi(ackResponse.back()) << std::endl;
							++replicasMetThreshold;
						}
					}
					catch(const std::exception& e)
					{
						FD_CLR(currSock, &replSockets);
					}
				}
			}

			//Check time threshold
			timeval t;
			gettimeofday(&t, nullptr);

			if (t.tv_sec > timeUntil.tv_sec || (t.tv_sec == timeUntil.tv_sec && t.tv_usec > timeUntil.tv_usec))
			{
				std::cout << "Timed out!" << std::endl;
				break;
			}
		}

		/*
		timeval timeUntil;
		gettimeofday(&timeUntil, nullptr);

		// set timeout
		timeUntil.tv_usec += timeThreshold * 1000;
		if (timeUntil.tv_usec > 1000000)
		{// update seconds
			timeUntil.tv_sec += timeUntil.tv_usec / 1000000;
			timeUntil.tv_usec = timeUntil.tv_usec % 1000000;
		}

		std::cout << "Got thresholds: " << replicaThreshold << " " << timeThreshold << std::endl;

		for (auto& replica : m_mapReplicaPortSocket)
		{
			try
			{
				std::cout << "Checking replica: " << replica.first << std::endl;
				// sendData(replica.second, {REPLCONF, "getack", "wait"});
				sendData(replica.second, {"REPLCONF", "GETACK", "*"});
				auto ackResponse{SocketReader(replica.second).ReadArray()};

				// if (ackResponse.size() == 3 && std::stoi(ackResponse.back()) == std::stoi(m_mapConfiguration["waitcmd_offset"]))
				if (ackResponse.size() == 3)
				{
					std::cout << "[Replica: " << replica.first << "] is up to date. Offset: " << std::stoi(ackResponse.back()) << std::endl;
					++replicasMetThreshold;

					// if (replicasMetThreshold == replicaThreshold)
					// {
					// 	std::cout << "Replica threshold met -> " << replicasMetThreshold << std::endl;
					// 	break;
					// }
				}
				else
					throw std::runtime_error("Not up to date");
			}
			catch (const std::exception& e)
			{
				std::cout << "[Replica: " << replica.first
					<< "] did not respond or offset not up to date. Error: " << e.what() << std::endl;
			}

			// Check time threshold
			// timeval t;
			// gettimeofday(&t, nullptr);

			// if (t.tv_sec >= timeUntil.tv_sec || t.tv_usec >= timeUntil.tv_usec)
			// {
			// 	std::cout << "Timed out!" << std::endl;
			// 	break;
			// }
		}
		*/

		return RESPEncoder::encodeInteger(replicasMetThreshold);
}

std::string CommandHandler::TYPE_cmdHandler(CommandArray commandArgs, KeyValueStore& kvStore, StreamHandler& streamHandler)
{
    if (commandArgs->size() != 2)
        return RESPEncoder::encodeError("wrong number of arguments for 'type' command");

    if (kvStore.get(commandArgs->at(1)) != NULL_BULK_ENCODED)
			return RESPEncoder::encodeSimpleString("string");
    else
    {
        if (streamHandler.IsStreamPresent(commandArgs->at(1)))
            return RESPEncoder::encodeSimpleString("stream");
    }

    return RESPEncoder::encodeSimpleString("none");
}

std::string CommandHandler::INCR_cmdHandler(CommandArray commandArgs, KeyValueStore &kvStore)
{
    if (commandArgs->size() != 2)
        return RESPEncoder::encodeError("wrong number of arguments for 'incr' command");

    std::string currentValue = kvStore.get(commandArgs->at(1));
    if (currentValue == NULL_BULK_ENCODED)
    {
        kvStore.set(commandArgs->at(1), "1");
        return RESPEncoder::encodeInteger(1);
    }
    else
    {
        try
        {
            int intValue = std::stoi(*RESPDecoder::decodeString(currentValue));
            intValue += 1;
            kvStore.set(commandArgs->at(1), std::to_string(intValue));
            return RESPEncoder::encodeInteger(intValue);
        }
        catch (const std::exception &e)
        {
            return RESPEncoder::encodeError("value is not an integer or out of range");
        }
    }

    return NULL_BULK_ENCODED; // null bulk string
}


