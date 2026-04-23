#include "Client.hpp"
#include "Server.hpp"
#include <iostream>

Client::Client(int fd, const std::string& ip, Server* server)
	: _fd(fd), _ip(ip), _server(server), _authenticated(false)
{
	(void)_server;
	std::cerr << "TODO: Client::Client" << std::endl;
}

Client::~Client()
{
	std::cerr << "TODO: Client::~Client" << std::endl;
}

int Client::getFd() const
{
	return _fd;
}

const std::string& Client::getNickname() const
{
	return _nickname;
}

const std::string& Client::getUsername() const
{
	return _username;
}

bool Client::isAuthenticated() const
{
	return _authenticated;
}

void Client::setNickname(const std::string& nick)
{
	(void)nick;
	std::cerr << "TODO: Client::setNickname" << std::endl;
}

void Client::setUsername(const std::string& user)
{
	(void)user;
	std::cerr << "TODO: Client::setUsername" << std::endl;
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
