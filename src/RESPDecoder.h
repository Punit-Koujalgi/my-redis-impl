
#ifndef _RESP_DECODER_
#define _RESP_DECODER_

#include <memory>
#include <string>
#include <vector>

class RESPDecoder
{

public:

	static std::unique_ptr<std::string> decodeString(const std::string& str);
	static std::unique_ptr<std::vector<std::string>> decodeArray(const std::string& str);

private:

	static std::string decodeSimpleString(const std::string& str);
	static std::string decodeBulkString(const std::string& str);

};

#endif