
#include "KeyValueStore.h"
#include "RESPEncoder.h"
#include "RESPDecoder.h"

#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <cassert>
#include <regex>

const std::string KeyValueStore::get(const std::string& key)
{
	if (m_mapKeyValues.find(key) == m_mapKeyValues.end())
		return NULL_BULK_ENCODED;

	if (m_mapKeyTimeouts.find(key) != m_mapKeyTimeouts.end())
	{
		// verify timeout has not expired
		timeVal t;
		gettimeofday(&t, NULL);

		if (m_mapKeyTimeouts[key].tv_sec < t.tv_sec)
			return std::string("$-1\r\n");
		else if (m_mapKeyTimeouts[key].tv_sec == t.tv_sec && m_mapKeyTimeouts[key].tv_usec < t.tv_usec)
			return std::string("$-1\r\n");

		std::cout << "Not expired" << std::endl;
	}

	return m_mapKeyValues[key];
}

std::unique_ptr<std::vector<std::string>> KeyValueStore::getArray(const std::string& key)
{
	if (m_mapKeyValues.find(key) == m_mapKeyValues.end())
		return std::make_unique<std::vector<std::string>>(); // null bulk string

	return RESPDecoder::decodeArray(m_mapKeyValues[key]);
}

const std::string KeyValueStore::set(const std::string& key, const std::string& value, int timeout)
{
	m_mapKeyValues[key] = RESPEncoder::encodeString(value);

	if (timeout != 0)
	{
		timeVal t;
		gettimeofday(&t, NULL);
		
		// increase timeout
		t.tv_usec += timeout * 1000;

		if (t.tv_usec > 1000000)
		{
			// update seconds
			t.tv_sec += t.tv_usec / 1000000;
			t.tv_usec = t.tv_usec % 1000000;
		}

		m_mapKeyTimeouts[key] = t;
	}

	return "+OK\r\n";
}

const std::string KeyValueStore::set(const std::string& key, const std::vector<std::string>& arrVal)
{
	m_mapKeyValues[key] = RESPEncoder::encodeArray(arrVal);
	return "+OK\r\n";
}


void KeyValueStore::initializeKeyValues(const std::string& dbpath, const std::string& dbfile)
{
	if (dbpath.empty() || dbfile.empty())
		return;

	std::string rdbFullPath = dbpath + "/" + dbfile;
	std::ifstream rdb(rdbFullPath, std::ios_base::binary);
	if (!rdb.is_open())
	{
		std::cout << "Could not open the Redis Persistent Database:"  << rdbFullPath << std::endl;
		return;
	}

	char header[9];
	rdb.read(header, 9);
	std::cout << "Header: " << std::string(header, 9) << std::endl; //always REDIS0011

	// metadata
	while (true)
	{
		unsigned char opcode;
		if (!rdb.read(reinterpret_cast<char*>(&opcode), 1))
			throw std::runtime_error("Reached end of file while looking for database start");

		if (opcode == 0xFA) // some metadata like version
		{
			std::string key = read_byte_to_string(rdb);
			std::string value = read_byte_to_string(rdb);
			std::cout << "Info: " << key << " " << value << std::endl;
		}

		if (opcode == 0xFE)
		{
			auto db_number = get_str_bytes_len(rdb);
			if (db_number.first.has_value())
			{
				std::cout << "SELECTDB: Database number: " << db_number.first.value() << std::endl;
				opcode = read(rdb); // Read next opcode
			}
		}

		if (opcode == 0xFB)
		{
			auto hash_table_size = get_str_bytes_len(rdb);
			auto expire_hash_table_size = get_str_bytes_len(rdb);

			if (hash_table_size.first.has_value() &&
				expire_hash_table_size.first.has_value())
			{
				std::cout << "Hash table size: " << hash_table_size.first.value() << " "
							<< "Expiry hash table size: " << expire_hash_table_size.first.value() << std::endl;
			}
			break;
		}
	}

	// Read key-value pairs
	while (true)
	{
		unsigned char opcode;
		if (!rdb.read(reinterpret_cast<char*>(&opcode), 1))
		{
			std::cout << "Reached end of file" << std::endl;
			break;
		}

		if (opcode == 0xFF)
		{
			std::cout << "Reached end of database" << std::endl;
			uint64_t checksum;
			rdb.read(reinterpret_cast<char*>(&checksum), sizeof(checksum));
			//checksum = be64toh(checksum); // be is big endian to host order
			std::cout << "DB checksum: " << checksum << std::endl;
			
			// Exit while loop
			break;
		}

		uint64_t expire_time_s = 0;
		uint64_t expire_time_ms = 0;
		if (opcode == 0xFD)
		{
			// expiry time in seconds followed by 4 byte - uint32_t
			uint32_t seconds;
			rdb.read(reinterpret_cast<char*>(&seconds), sizeof(seconds));
			//expire_time_s = be32toh(seconds);
			std::cout << "EXPIRETIME: " << expire_time_s << std::endl;

			rdb.read(reinterpret_cast<char*>(&opcode), 1);
		}

		if  (opcode == 0xFC)
		{
			// expiry time in ms, followd by 8 byte unsigned - uint64_t
			rdb.read(reinterpret_cast<char*>(&expire_time_ms), sizeof(expire_time_ms));
			//expire_time_ms = be64toh(expire_time_ms);
			std::cout << "EXPIRETIME ms: " << expire_time_ms << std::endl;

			rdb.read(reinterpret_cast<char*>(&opcode), 1);
		}

		// After 0xFD and 0x FC, comes the key-pair-value
		std::string key = read_byte_to_string(rdb);
		std::string value = read_byte_to_string(rdb);

		// Add KV pair if it hasn't expired
		timeVal t;
		gettimeofday(&t, NULL);

		if (expire_time_s == 0 || t.tv_sec < expire_time_s)
		{
			std::cout << "Adding " << key << " -> " << value << std::endl;
			set(key, value, expire_time_ms);

			// Overwrite value based on fact that expire_time_ms is unix time stamp for expiry in ms
			if (expire_time_ms != 0)
			{
				t.tv_sec = expire_time_ms / 1000;
				t.tv_usec = (expire_time_ms % 1000) * 1000;

				m_mapKeyTimeouts[key] = t;
			}
		}

	}

	rdb.close();
}


uint8_t KeyValueStore::read(std::ifstream &rdb)
{
	uint8_t val;
	rdb.read(reinterpret_cast<char *>(&val), sizeof(val));
	return val;
}

std::pair<std::optional<uint64_t>, std::optional<int8_t>> KeyValueStore::get_str_bytes_len(std::ifstream &rdb) {
	// Read the first byte from the RDB file
	auto byte = read(rdb);
	// Get the two most significant bits of the byte
	// These bits determine how the length is encoded
	auto sig = byte >> 6; // 0 bytes, 1, 2, 3 - 00, 01, 10, 11
	switch (sig) {
		case 0:
		{
			// If the two most significant bits are 00
			// The length is the lower 6 bits of the byte
			return {byte & 0x3F, std::nullopt};
		}
		case 1:
		{
			// If the two most significant bits are 01
			// The length is the lower 6 bits of the first byte and the whole next byte
			auto next_byte = read(rdb);
			uint64_t sz = ((byte & 0x3F) << 8) | next_byte;
			return {sz, std::nullopt};
		}
		case 2:
		{
			// If the two most significant bits are 10
			// The length is the next 4 bytes
			uint64_t sz = 0;
			for (int i = 0; i < 4; i++) {
				auto byte = read(rdb);
				sz = (sz << 8) | byte;
			}
			return {sz, std::nullopt};
		}
		case 3:
		{
			// If the two most significant bits are 11
			// The string is encoded as an integer
			switch (byte)
			{
			case 0xC0:
				// The string is encoded as an 8-bit integer of 1 byte
				return {std::nullopt, 8};
			case 0xC1:
				// The string is encoded as a 16-bit integer of 2 bytes
				return {std::nullopt, 16};
			case 0xC2:
				// The string is encoded as a 32-bit integer of 4 bytes
				return {std::nullopt, 32};
			case 0xFD:
				// Special case for database sizes
				return {byte, std::nullopt};
			default:
				return {std::nullopt, 0};
			}
		}
	}
	return {std::nullopt, 0};
}

std::string KeyValueStore::read_byte_to_string(std::ifstream &rdb)
{
	std::pair<std::optional<uint64_t>, std::optional<int8_t>> decoded_size = get_str_bytes_len(rdb);

	if (decoded_size.first.has_value()) // the length of the string is prefixed
	{
		int size = decoded_size.first.value();
		std::vector<char> buffer(size);
		rdb.read(buffer.data(), size);
		return std::string(buffer.data(), size);
	}

	assert(decoded_size.second.has_value());

	int type = decoded_size.second.value();

	switch(type)
	{
		case 8:
		{
			int8_t val;
			rdb.read(reinterpret_cast<char *>(&val), sizeof(val));
			return std::to_string(val);
		}
		case 16:
		{
			int16_t val;
			rdb.read(reinterpret_cast<char *>(&val), sizeof(val));
			//val = be16toh(val);
			return std::to_string(val);
		}
		case 32:
		{
			int32_t val;
			rdb.read(reinterpret_cast<char *>(&val), sizeof(val));
			//val = be32toh(val);
			return std::to_string(val);
		}
	}

	return "";
}

std::unique_ptr<std::vector<std::string>> KeyValueStore::getAllKeys(const std::string& regex)
{
	auto result{std::make_unique<std::vector<std::string>>()};

	std::cout << "Got regex: " << regex << std::endl;

	// If regex is empty or "*", return all keys
	if (regex.empty() || regex == "*")
	{
		for (const auto& key: m_mapKeyValues)
		{
			result->push_back(key.first);
		}
		return result;
	}

	try 
	{
		// Convert Redis-style glob pattern to regex if needed
		std::string regexPattern = regex;
		
		// Simple glob-to-regex conversion for Redis KEYS command compatibility
		// Replace * with .* and ? with .
		std::string convertedPattern;
		for (char c : regexPattern) 
		{
			switch (c) 
			{
				case '*':
					convertedPattern += ".*";
					break;
				case '?':
					convertedPattern += ".";
					break;
				case '.':
				case '^':
				case '$':
				case '+':
				case '{':
				case '}':
				case '(':
				case ')':
				case '[':
				case ']':
				case '\\':
				case '|':
					// Escape regex special characters
					convertedPattern += "\\";
					convertedPattern += c;
					break;
				default:
					convertedPattern += c;
					break;
			}
		}
		
		std::regex pattern(convertedPattern);

		// Filter keys that match the regex pattern
		for (const auto& key: m_mapKeyValues)
		{
			if (std::regex_match(key.first, pattern))
			{
				result->push_back(key.first);
			}
		}
	}
	catch (const std::regex_error& e)
	{
		std::cout << "Invalid regex pattern: " << regex << " Error: " << e.what() << std::endl;
		// Return empty result for invalid regex
	}

	return result;
}

