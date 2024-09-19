
#include "KeyValueStore.h"
#include "RESPEncoder.h"
#include "RESPDecoder.h"

const std::string KeyValueStore::get(const std::string& key)
{
	if (m_mapKeyValues.find(key) == m_mapKeyValues.end())
		return std::string("$-1\r\n"); // null bulk string

	return m_mapKeyValues[key];
}

std::unique_ptr<std::vector<std::string>> KeyValueStore::getArray(const std::string& key)
{
	if (m_mapKeyValues.find(key) == m_mapKeyValues.end())
		return std::make_unique<std::vector<std::string>>(); // null bulk string

	return RESPDecoder::decodeArray(m_mapKeyValues[key]);
}

const std::string KeyValueStore::set(const std::string& key, const std::string& value)
{
	m_mapKeyValues[key] = RESPEncoder::encodeString(value);
	return "+OK\r\n";
}

const std::string KeyValueStore::set(const std::string& key, const std::vector<std::string>& arrVal)
{
	m_mapKeyValues[key] = RESPEncoder::encodeArray(arrVal);
	return "+OK\r\n";
}

