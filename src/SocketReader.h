
#ifndef _SOCKET_READER_H_
#define _SOCKET_READER_H_

#include <stdint.h>
#include <string>
#include <vector>

class SocketReader
{
	int m_fd;
	/* Read only one command at a time as multiple commands might be sent in the same message */
	uint8_t readByte();
	void readSlashRN();

public:
	
	SocketReader(int fd) : m_fd(fd){}

	std::string readSimpleString(); 
	std::string readBulkString();
	std::vector<std::string> ReadArray();

	std::string readRDBFile();

};

#endif