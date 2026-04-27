#include "Channel.hpp"
#include "Client.hpp"
#include <iostream>

Channel::Channel(const std::string& name, Client* creator)
	: _name(name), _topic(""), _inviteOnly(false), _key(""), _userLimit(0)
{
	(void)creator;
	std::cerr << "TODO: Channel::Channel" << std::endl;
}

Channel::~Channel()
{
	std::cerr << "TODO: Channel::~Channel" << std::endl;
}

const std::string& Channel::getName() const
{
	return _name;
}

const std::string& Channel::getTopic() const
{
	return _topic;
}

void Channel::setTopic(std::string topic)
{
	(void)topic;
	std::cerr << "TODO: Channel::setTopic" << std::endl;
}

bool Channel::isInviteOnly() const
{
	return _inviteOnly;
}

void Channel::setInviteOnly(bool inviteOnly)
{
	_inviteOnly = inviteOnly;
}

bool Channel::hasKey() const
{
	return !_key.empty();
}

bool Channel::checkKey(const std::string& key) const
{
	(void)key;
	std::cerr << "TODO: Channel::checkKey" << std::endl;
	return true;
}

void Channel::setKey(std::string key)
{
	_key = key;
	std::cerr << "TODO: Channel::setKey" << std::endl;
}

int Channel::getUserLimit() const
{
	return _userLimit;
}

void Channel::setUserLimit(int limit)
{
	_userLimit = limit;
}

void Channel::addClient(Client* client)
{
	(void)client;
	std::cerr << "TODO: Channel::addClient" << std::endl;
}

void Channel::removeClient(Client* client)
{
	(void)client;
	std::cerr << "TODO: Channel::removeClient" << std::endl;
}

bool Channel::isOperator(Client* client) const
{
	(void)client;
	std::cerr << "TODO: Channel::isOperator" << std::endl;
	return false;
}

void Channel::addOperator(Client* client)
{
	(void)client;
	std::cerr << "TODO: Channel::addOperator" << std::endl;
}

void Channel::removeOperator(Client* client)
{
	(void)client;
	std::cerr << "TODO: Channel::removeOperator" << std::endl;
}

void Channel::broadcastMessage(const std::string& msg, Client* sender)
{
	(void)msg;
	(void)sender;
	std::cerr << "TODO: Channel::broadcastMessage" << std::endl;
}

std::vector<Client*> Channel::getClients() const
{
	return _clients;
}
