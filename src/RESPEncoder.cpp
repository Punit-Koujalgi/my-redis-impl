
#include "RESPEncoder.h"

const std::string RESPEncoder::encodeString(const std::string& str)
{
	std::string result{"$"};
	result.append(std::to_string(str.length()));
	result.append("\r\n");
	result.append(str);
	result.append("\r\n");

	return result;
}

const std::string RESPEncoder::encodeSimpleString(const std::string& str)
{
	std::string result{"+"};
	result.append(str);
	result.append("\r\n");

	return result;
}

const std::string RESPEncoder::encodeArray(const std::vector<std::string>& arr)
{
	std::string result{"*"};
	result.append(std::to_string(arr.size()));
	result.append("\r\n");

	for (auto& str : arr)
	{
		result.append(encodeString(str));
	}

	return result;
}


