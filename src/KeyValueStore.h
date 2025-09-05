
#ifndef _KEY_VALUE_STORE_H_
#define _KEY_VALUE_STORE_H_

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>

typedef struct timeval timeVal;

/*

	key - value store
	- support expiration
	- support different types

*/

#define NULL_BULK_ENCODED "$-1\r\n"

class KeyValueStore
{

public:

	void initializeKeyValues(const std::string& dbpath, const std::string& dbfile);

	const std::string get(const std::string& key);
	std::unique_ptr<std::vector<std::string>> getArray(const std::string& key);

	const std::string set(const std::string& key, const std::string& value, int timeout = 0);
	const std::string set(const std::string& key, const std::vector<std::string>& arrVal);

	std::unique_ptr<std::vector<std::string>> getAllKeys(const std::string& regex = "");

private:

	std::unordered_map<std::string, std::string> m_mapKeyValues;
	std::unordered_map<std::string, timeVal> m_mapKeyTimeouts;


	unsigned char read(std::ifstream &rdb);
	std::pair<std::optional<uint64_t>, std::optional<int8_t>> get_str_bytes_len(std::ifstream &rdb);
	std::string read_byte_to_string(std::ifstream &rdb);

};


#endif

