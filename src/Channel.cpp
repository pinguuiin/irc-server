#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include <algorithm>

Channel::Channel(const std::string& name, Client* creator, Server* server)
	: _name(name), _topic(""), _inviteOnly(false), _key(""), _topicRestricted(true), _userLimit(0), _server(server)
{
	if (creator != NULL) {
		_clients.push_back(creator);
		_operators.push_back(creator);
	}
}

Channel::~Channel()
{
}

const std::string& Channel::getName() const
{
	return _name;
}

const std::string& Channel::getTopic() const
{
	return _topic;
}

const std::string& Channel::getKey() const
{
	return _key;
}

void Channel::setTopic(std::string topic)
{
	_topic = topic;
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
	if (!hasKey())
		return true;
	return _key == key;
}

void Channel::setKey(std::string key)
{
	_key = key;
}

bool Channel::isTopicRestricted() const
{
	return _topicRestricted;
}

void Channel::setTopicRestricted(bool restricted)
{
	_topicRestricted = restricted;
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
	if (client == NULL || hasClient(client))
		return;
	if (_userLimit > 0 && static_cast<int>(_clients.size()) >= _userLimit)
		return;
	_clients.push_back(client);
	clearInvite(client);
}

void Channel::removeClient(Client* client)
{
	if (client == NULL)
		return;
	_clients.erase(std::remove(_clients.begin(), _clients.end(), client), _clients.end());
	_operators.erase(std::remove(_operators.begin(), _operators.end(), client), _operators.end());
	_invited.erase(std::remove(_invited.begin(), _invited.end(), client), _invited.end());
	if (_clients.empty())
		_server->removeChannel(_name);
}

bool Channel::hasClient(Client* client) const
{
	if (client == NULL)
		return false;
	return std::find(_clients.begin(), _clients.end(), client) != _clients.end();
}

bool Channel::isOperator(Client* client) const
{
	if (client == NULL)
		return false;
	return std::find(_operators.begin(), _operators.end(), client) != _operators.end();
}

void Channel::addOperator(Client* client)
{
	if (client == NULL || !hasClient(client) || isOperator(client))
		return;
	_operators.push_back(client);
}

void Channel::removeOperator(Client* client)
{
	if (client == NULL)
		return;
	_operators.erase(std::remove(_operators.begin(), _operators.end(), client), _operators.end());
}

void Channel::inviteClient(Client* client)
{
	if (client == NULL)
		return;
	if (std::find(_invited.begin(), _invited.end(), client) == _invited.end())
		_invited.push_back(client);
}

bool Channel::isInvited(Client* client) const
{
	if (client == NULL)
		return false;
	return std::find(_invited.begin(), _invited.end(), client) != _invited.end();
}

void Channel::clearInvite(Client* client)
{
	if (client == NULL)
		return;
	_invited.erase(std::remove(_invited.begin(), _invited.end(), client), _invited.end());
}

void Channel::broadcastMessage(const std::string& msg, Client* sender)
{
	for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (*it != NULL && *it != sender && _server != NULL)
			_server->queueMessage((*it)->getFd(), msg);
	}
}

std::vector<Client*> Channel::getClients() const
{
	return _clients;
}
