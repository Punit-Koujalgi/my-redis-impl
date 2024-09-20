
#ifndef _KEY_VALUE_STORE_H_
#define _KEY_VALUE_STORE_H_

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

typedef struct timeval timeVal;

/*

	key - value store
	- support expiration
	- support different types

*/

class KeyValueStore
{

public:

	const std::string get(const std::string& key);
	std::unique_ptr<std::vector<std::string>> getArray(const std::string& key);

	const std::string set(const std::string& key, const std::string& value, int timeout = 0);
	const std::string set(const std::string& key, const std::vector<std::string>& arrVal);

private:

	std::unordered_map<std::string, std::string> m_mapKeyValues;
	std::unordered_map<std::string, timeVal> m_mapKeyTimeouts;
};


#endif

