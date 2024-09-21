
#include "SocketReader.h"

#include <iostream>
#include <unistd.h>	// for read(), close()
#include <cassert>
#include <exception>

uint8_t SocketReader::readByte()
{
	uint8_t val;
	int n = read(m_fd, &val, sizeof(val));
	if (n < 0)
		throw std::runtime_error("read() failed");
	else if (n == 0)
		std::cout << "EOF reached" << std::endl;
 
	return val;
};

void SocketReader::readSlashRN()
{
	readByte(); readByte(); // for \r\n
}

std::string SocketReader::readSimpleString()
{
	assert(readByte() == '+'); // for +
	std::string res;
	char c;
	while ((c = (char)readByte()) != '\r')
		res.push_back(c);
	readByte(); // for \n 
	return res;
}

std::string SocketReader::readBulkString()
{
	assert(readByte() == '$'); // for $
	
	std::string leng;
	char c;
	while ((c = readByte()) != '\r')
		leng.push_back(c);

	int length = std::stoi(leng);
	readByte(); // for \n 

	std::vector<char> buffer(length);
	read(m_fd, buffer.data(), length);
	readSlashRN();
	return std::string(buffer.data(), length);
};

std::vector<std::string> SocketReader::ReadArray()
{
	assert(readByte() == '*'); // for *
	readSlashRN();

	std::vector<std::string> res;

	std::string leng;
	char c;
	while ((c = readByte()) != '\r')
		leng.push_back(c);

	int length = std::stoi(leng);
	readByte(); // for \n 

	for (int i = 0; i < length; ++i)
	{
		res.push_back(readBulkString());
	}

	return res;
}

std::string SocketReader::readRDBFile()
{
	assert(readByte() == '$'); // for $
	
	std::string leng;
	char c;
	while ((c = readByte()) != '\r')
		leng.push_back(c);

	int length = std::stoi(leng);
	readByte(); // for \n 

	std::vector<char> buffer(length);
	read(m_fd, buffer.data(), length);

	return std::string(buffer.data(), length);	
}
