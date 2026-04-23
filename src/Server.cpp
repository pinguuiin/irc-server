#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include <iostream>

Server::Server(int port, const std::string& password)
	: _port(port), _password(password), _running(false)
{
	std::cerr << "TODO: Server::Server" << std::endl;
}

Server::~Server()
{
	std::cerr << "TODO: Server::~Server" << std::endl;
}

void Server::run()
{
	_running = true;
	std::cerr << "TODO: Server::run (port " << _port << ")" << std::endl;
	_running = false;
}

void Server::stop()
{
	std::cerr << "TODO: Server::stop" << std::endl;
	_running = false;
}

void Server::addClient(int fd, const std::string& ip)
{
	(void)fd;
	(void)ip;
	std::cerr << "TODO: Server::addClient" << std::endl;
}

void Server::removeClient(int fd)
{
	(void)fd;
	std::cerr << "TODO: Server::removeClient" << std::endl;
}

Client* Server::getClient(int fd)
{
	(void)fd;
	std::cerr << "TODO: Server::getClient" << std::endl;
	return NULL;
}

Channel* Server::getChannel(const std::string& name)
{
	(void)name;
	std::cerr << "TODO: Server::getChannel" << std::endl;
	return NULL;
}

Channel* Server::createChannel(const std::string& name, Client* creator)
{
	(void)name;
	(void)creator;
	std::cerr << "TODO: Server::createChannel" << std::endl;
	return NULL;
}

const std::string& Server::getPassword() const
{
	return _password;
}
