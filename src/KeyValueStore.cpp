
#include "KeyValueStore.h"
#include "RESPEncoder.h"
#include "RESPDecoder.h"

#include <iostream>
#include <sys/time.h>

const std::string KeyValueStore::get(const std::string& key)
{
	if (m_mapKeyValues.find(key) == m_mapKeyValues.end())
		return std::string("$-1\r\n"); // null bulk string

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

