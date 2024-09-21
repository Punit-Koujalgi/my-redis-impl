
#include "RESPDecoder.h"

#include <iostream>
#include <exception>


std::unique_ptr<std::string> RESPDecoder::decodeString(const std::string& str)
{
	if (str.empty())
		throw std::runtime_error("Empty string passed"); 

	if(str[0] == '+')
		return std::make_unique<std::string>(decodeSimpleString(str));
	else if (str[0] == '$')
		return std::make_unique<std::string>(decodeBulkString(str));
	else
		throw std::runtime_error(std::string("Type [") + str[0] + std::string("] not yet handled"));
}

std::string RESPDecoder::decodeSimpleString(const std::string& str)
{
	if (str.length() < 3)
		throw std::runtime_error("Malformed input string for decodeSimpleString()");

	return str.substr(1, str.find('\r') - 1); // len is end - start + 1
}

std::string RESPDecoder::decodeBulkString(const std::string& str)
{
	// empty string: $0\r\n\r\n

	if (str.length() < 6)
		throw std::runtime_error("Malformed input string for decodeBulkString()");

	int actualLen = stoi(str.substr(1, str.find('\r') - 1));

	if (actualLen == -1)
		return "NULL";

	return str.substr(str.find('\n') + 1, actualLen);
}

std::unique_ptr<std::vector<std::string>> RESPDecoder::decodeArray(const std::string& str)
{
	// empty array: *0\r\n
	if (str.length() < 4)
		throw std::runtime_error("Malformed input string for decodeArrays()");

	auto result{std::make_unique<std::vector<std::string>>()};

	int prevrn = str.find('\n');
	int numElements = stoi(str.substr(1, (prevrn - 1) - 1)); // -1 for \r

	for (int index{0}; index < numElements; ++index)
	{
		int nextrn = str.find('\n', prevrn + 1);
		std::string element;
		if (str[prevrn + 1] == '$')
		{
			nextrn = str.find('\n', nextrn + 1); // consume two lines
			element = str.substr(prevrn + 1, nextrn - prevrn);
		}
		else if(str[prevrn + 1] == '+')
		{
			element = str.substr(prevrn + 1, nextrn - prevrn);
		}

		result->push_back(decodeBulkString(element));
		prevrn = nextrn;
	}

	return result;
}


