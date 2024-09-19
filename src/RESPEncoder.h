
#ifndef _RESP_ENCODER_H_
#define _RESP_ENCODER_H_

#include <memory>
#include <vector>
#include <string>

class RESPEncoder
{

public:

	static const std::string encodeString(const std::string& str);
	static const std::string encodeArray(const std::vector<std::string>& arr);
};

#endif

