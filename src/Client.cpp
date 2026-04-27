#include "Client.hpp"
#include "Server.hpp"
#include <iostream>

Client::Client(int fd, std::string ip, Server* server)
	: _fd(fd), _ip(ip), _server(server), _authenticated(false)
{
	(void)_server;
}

Client::~Client()
{
	std::cerr << "TODO: Client::~Client" << std::endl;
}

const int& Client::getFd() const
{
	return _fd;
}

const std::string& Client::getIp() const
{
	return _ip;
}

const std::string& Client::getNickname() const
{
	return _nickname;
}

const std::string& Client::getUsername() const
{
	return _username;
}

void Client::setFd(int fd)
{
	_fd = fd;
}

void Client::setIp(std::string ip)
{
	_ip = ip;
}

void Client::setNickname(std::string nick)
{
	_nickname = nick;
}

void Client::setUsername(std::string user)
{
	_username = user;
}

bool Client::isAuthenticated() const
{
	return _authenticated;
}

void Client::authenticate()
{
	std::cerr << "TODO: Client::authenticate" << std::endl;
}

void Client::sendMessage(const std::string& msg)
{
	(void)msg;
	std::cerr << "TODO: Client::sendMessage" << std::endl;
}

void Client::receiveData(const std::string& data)
{
	(void)data;
	std::cerr << "TODO: Client::receiveData" << std::endl;
}

std::string Client::getNextMessage()
{
	std::cerr << "TODO: Client::getNextMessage" << std::endl;
	return std::string();
}
