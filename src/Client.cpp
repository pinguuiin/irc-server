#include "../include/Client.hpp"
#include <unistd.h> // for close()
#include <cstring> // for strerror(), memset()
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct sockaddr
#include <iostream> // for io and error handling

// Ping's originals
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

void Client::setIp(const std::string &ip)
{
	_ip = ip;
}

// Auth state
bool Client::hasPassOk() const
{
	return _passOk;
}

bool Client::isRegistered() const
{
	return _registered;
}

bool Client::isOper() const
{
	return _isOper;
}

void Client::setPassOk(bool v)
{
	_passOk = v;
}

void Client::setRegistered(bool v)
{
	_registered = v;
}

void Client::setOper(bool v)
{
	_isOper = v;
}


// Identity
const std::string &Client::getNick() const
{
	return _nick;
}

const std::string &Client::getUsername() const
{
	return _username;
}

const std::string &Client::getRealname() const
{
	return _realname;
}

void Client::setNick(const std::string &n)
{
	_nick = n;
}

void Client::setUsername(const std::string &u)
{
	_username = u;
}

void Client::setRealname(const std::string &r)
{
	_realname = r;
}

// Buffer
std::string &Client::getMsgBuffer()
{
	return _msgBuffer;
}
