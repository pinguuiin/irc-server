#include "../include/Client.hpp"
#include <unistd.h> // for close()
#include <cstring> // for strerror(), memset()
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct sockaddr
#include <iostream> // for io and error handling


const int &Client::getFd() const
{
	return _fd;
}

const std::string &Client::getIp() const
{
	return _ip;
}

void Client::setFd(int fd)
{
	_fd = fd;
}

void Client::setIp(std::string ip)
{
	_ip = ip;
}
