#include "CommandHandler.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <cstdlib>

namespace {
	static std::string clientMask(Client* client)
	{
		std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
		std::string user = client->getUsername().empty() ? "unknown" : client->getUsername();
		return nick + "!" + user + "@localhost";
	}

	static std::string errUnknownCommand(const std::string& nick, const std::string& cmd)
	{
		return ":ircserv 421 " + nick + " " + cmd + " :Unknown command\r\n";
	}

	static std::string errNeedMoreParams(const std::string& nick, const std::string& cmd)
	{
		return ":ircserv 461 " + nick + " " + cmd + " :Not enough parameters\r\n";
	}

	static std::string errNoSuchChannel(const std::string& nick, const std::string& channelName)
	{
		return ":ircserv 403 " + nick + " " + channelName + " :No such channel\r\n";
	}

	static std::string errNotOnChannel(const std::string& nick, const std::string& channelName)
	{
		return ":ircserv 442 " + nick + " " + channelName + " :You're not on that channel\r\n";
	}

	static std::string errChanOpPrivsNeeded(const std::string& nick, const std::string& channelName)
	{
		return ":ircserv 482 " + nick + " " + channelName + " :You're not channel operator\r\n";
	}
}

CommandHandler::CommandHandler(Server* server)
	: _server(server)
{
}

void CommandHandler::handleCommand(Client* client, const CommandParser::ParsedCommand& cmd)
{
	std::string command = Utils::toUpper(cmd.command);
	std::vector<std::string> params = cmd.params;
	if (!cmd.trailing.empty())
		params.push_back(cmd.trailing);
	if (command == "PASS")
		handlePass(client, params);
	else if (command == "NICK")
		handleNick(client, params);
	else if (command == "USER")
		handleUser(client, params);
	else if (command == "JOIN")
		handleJoin(client, params);
	else if (command == "PRIVMSG")
		handlePrivmsg(client, params);
	else if (command == "KICK")
		handleKick(client, params);
	else if (command == "INVITE")
		handleInvite(client, params);
	else if (command == "TOPIC")
		handleTopic(client, params);
	else if (command == "MODE")
		handleMode(client, params);
	else
		_server->queueMessage(client->getFd(), errUnknownCommand(client->getNickname(), command));
}

// ── handlePass ──────────────────────────────────────────────────────
// Validates the servver password and marks _passOk on the client.
// Then tries to complete registration n case NICK+USER already arrived.
void CommandHandler::handlePass(Client* client, const std::vector<std::string>& params)
{
	if (params.empty()) {
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "PASS"));
		return;
	}
	if (params[0] != _server->getPassword())
	{
		_server->queueMessage(client->getFd(), ":ircserv 464 " + client->getNickname() + " :Password incorrect\r\n");
		return;
	}
	client->setPassOk();
	tryCompleteRegistration(client);
}

// ── handleNick ──────────────────────────────────────────────────────
// Sets nickname after checking validity and uniqueness (433 ERR_NICKNAMEINUSE).
// Marks _nickSet and tries to complete registration.
void CommandHandler::handleNick(Client* client, const std::vector<std::string>& params)
{
	if (params.empty()) {
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "NICK"));
		return;
	}
	if (!Utils::isValidNickname(params[0])) {
		_server->queueMessage(client->getFd(), ":ircserv 432 * " + params[0] + " :Erroneous nickname\r\n");
		return;
	}
	// 433 ERR_NICKNAMEINUSE - reject if another client already holds this nick
	Client* existing = _server->getClientByNickname(params[0]);
	if (existing != NULL && existing != client)
	{
		_server->queueMessage(client->getFd(), ":ircserv 433 * " + params[0] + " :Nickname is already in use\r\n");
		return;
	}
	client->setNickname(params[0]);
	client->setNickSet();
	tryCompleteRegistration(client);
}

// ── handleUser ──────────────────────────────────────────────────────
// Sets the username (ident). Marks _userSet and tries to complete registration.
void CommandHandler::handleUser(Client* client, const std::vector<std::string>& params)
{
	if (params.empty()) {
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "USER"));
		return;
	}
	client->setUsername(params[0]);
	client->setUserSet();
	tryCompleteRegistration(client);
}

// ── tryCompleteRegistration ─────────────────────────────────────────
// Called after every PASS/NICK/USER handler.
// Sends 001-004 numerics once all three steps are done.
// irssi waits for 001 RPL_WELCOME before considering itself connected.
void CommandHandler::tryCompleteRegistration(Client* client)
{
	if (!client->isPassOk() || !client->isNickSet() || !client->isUserSet())
		return;
	if (client->isAuthenticated()) // guard: don't send 001 twice
		return;
	client->authenticate();
	_server->queueMessage(client->getFd(), ":ircserv 001 " + client->getNickname() + " :Welcome to the IRC Network "
		+ client->getNickname() + "!" + client->getUsername() + "@localhost\r\n");

	_server->queueMessage(client->getFd(), ":ircserv 002 " + client->getNickname() + " :Your host is ircserv, running version 1.0\r\n");

	_server->queueMessage(client->getFd(), ":ircserv 003 " + client->getNickname() + " :This server was created just now\r\n");

	_server->queueMessage(client->getFd(), ":ircserv 004 " + client->getNickname() + " ircserv 1.0 o itkol\r\n");
}

void CommandHandler::handleJoin(Client* client, const std::vector<std::string>& params)
{
	if (params.empty()) {
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "JOIN"));
		return;
	}
	const std::string& channelName = params[0];
	if (!Utils::isValidChannelName(channelName)) {
		_server->queueMessage(client->getFd(), ":ircserv 476 " + client->getNickname() + " " + channelName + " :Bad Channel Mask\r\n");
		return;
	}

	Channel* channel = _server->getChannel(channelName);
	if (channel == NULL)
		channel = _server->createChannel(channelName, client);
	if (channel->hasClient(client))
		return;

	std::string key = params.size() > 1 ? params[1] : "";
	if (channel->isInviteOnly() && !channel->isInvited(client)) {
		_server->queueMessage(client->getFd(), ":ircserv 473 " + client->getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
		return;
	}
	if (channel->hasKey() && !channel->checkKey(key)) {
		_server->queueMessage(client->getFd(), ":ircserv 475 " + client->getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");
		return;
	}
	if (channel->getUserLimit() > 0 && static_cast<int>(channel->getClients().size()) >= channel->getUserLimit()) {
		_server->queueMessage(client->getFd(), ":ircserv 471 " + client->getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");
		return;
	}

	channel->addClient(client);
	std::string joinMsg = ":" + clientMask(client) + " JOIN :" + channelName + "\r\n";
	channel->broadcastMessage(joinMsg, client);
	_server->queueMessage(client->getFd(), joinMsg);
}

void CommandHandler::handlePrivmsg(Client* client, const std::vector<std::string>& params)
{
	if (params.size() < 2) {
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "PRIVMSG"));
		return;
	}
	const std::string& target = params[0];
	const std::string& text = params[1];

	if (!target.empty() && target[0] == '#') {
		Channel* channel = _server->getChannel(target);
		if (channel == NULL) {
			_server->queueMessage(client->getFd(), errNoSuchChannel(client->getNickname(), target));
			return;
		}
		if (!channel->hasClient(client)) {
			_server->queueMessage(client->getFd(), errNotOnChannel(client->getNickname(), target));
			return;
		}
		std::string msg = ":" + clientMask(client) + " PRIVMSG " + target + " :" + text + "\r\n";
		channel->broadcastMessage(msg, client);
		return;
	}

	Client* dst = _server->getClientByNickname(target);
	if (dst == NULL) {
		_server->queueMessage(client->getFd(), ":ircserv 401 " + client->getNickname() + " " + target + " :No such nick\r\n");
		return;
	}
	_server->queueMessage(dst->getFd(), ":" + clientMask(client) + " PRIVMSG " + target + " :" + text + "\r\n");
}

void CommandHandler::handleKick(Client* client, const std::vector<std::string>& params)
{
	if (params.size() < 2) {
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "KICK"));
		return;
	}
	Channel* channel = _server->getChannel(params[0]);
	if (channel == NULL) {
		_server->queueMessage(client->getFd(), errNoSuchChannel(client->getNickname(), params[0]));
		return;
	}
	if (!channel->isOperator(client)) {
		_server->queueMessage(client->getFd(), errChanOpPrivsNeeded(client->getNickname(), params[0]));
		return;
	}

	Client* target = _server->getClientByNickname(params[1]);
	if (target == NULL || !channel->hasClient(target)) {
		_server->queueMessage(client->getFd(), ":ircserv 441 " + client->getNickname() + " " + params[1] + " " + params[0] + " :They aren't on that channel\r\n");
		return;
	}
	std::string reason = params.size() > 2 ? params[2] : client->getNickname();
	std::string kickMsg = ":" + clientMask(client) + " KICK " + params[0] + " " + params[1] + " :" + reason + "\r\n";
	channel->broadcastMessage(kickMsg, NULL);
	_server->queueMessage(target->getFd(), kickMsg);
	channel->removeClient(target);
}

void CommandHandler::handleInvite(Client* client, const std::vector<std::string>& params)
{
	if (params.size() < 2) {
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "INVITE"));
		return;
	}
	Client* target = _server->getClientByNickname(params[0]);
	if (target == NULL) {
		_server->queueMessage(client->getFd(), ":ircserv 401 " + client->getNickname() + " " + params[0] + " :No such nick\r\n");
		return;
	}

	Channel* channel = _server->getChannel(params[1]);
	if (channel == NULL) {
		_server->queueMessage(client->getFd(), errNoSuchChannel(client->getNickname(), params[1]));
		return;
	}
	if (!channel->hasClient(client)) {
		_server->queueMessage(client->getFd(), errNotOnChannel(client->getNickname(), params[1]));
		return;
	}
	if (!channel->isOperator(client)) {
		_server->queueMessage(client->getFd(), errChanOpPrivsNeeded(client->getNickname(), params[1]));
		return;
	}

	channel->inviteClient(target);
	_server->queueMessage(target->getFd(), ":" + clientMask(client) + " INVITE " + target->getNickname() + " :" + params[1] + "\r\n");
	_server->queueMessage(client->getFd(), ":ircserv 341 " + client->getNickname() + " " + target->getNickname() + " " + params[1] + "\r\n");
}

void CommandHandler::handleTopic(Client* client, const std::vector<std::string>& params)
{
	if (params.empty()) {
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "TOPIC"));
		return;
	}
	Channel* channel = _server->getChannel(params[0]);
	if (channel == NULL) {
		_server->queueMessage(client->getFd(), errNoSuchChannel(client->getNickname(), params[0]));
		return;
	}
	if (!channel->hasClient(client)) {
		_server->queueMessage(client->getFd(), errNotOnChannel(client->getNickname(), params[0]));
		return;
	}

	if (params.size() == 1) {
		if (channel->getTopic().empty())
			_server->queueMessage(client->getFd(), ":ircserv 331 " + client->getNickname() + " " + params[0] + " :No topic is set\r\n");
		else
			_server->queueMessage(client->getFd(), ":ircserv 332 " + client->getNickname() + " " + params[0] + " :" + channel->getTopic() + "\r\n");
		return;
	}

	if (channel->isTopicRestricted() && !channel->isOperator(client)) {
		_server->queueMessage(client->getFd(), errChanOpPrivsNeeded(client->getNickname(), params[0]));
		return;
	}
	channel->setTopic(params[1]);
	channel->broadcastMessage(":" + clientMask(client) + " TOPIC " + params[0] + " :" + params[1] + "\r\n", NULL);
	_server->queueMessage(client->getFd(), ":" + clientMask(client) + " TOPIC " + params[0] + " :" + params[1] + "\r\n");
}

void CommandHandler::handleMode(Client* client, const std::vector<std::string>& params)
{
	if (params.size() < 2) {
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "MODE"));
		return;
	}
	Channel* channel = _server->getChannel(params[0]);
	if (channel == NULL) {
		_server->queueMessage(client->getFd(), errNoSuchChannel(client->getNickname(), params[0]));
		return;
	}
	if (!channel->isOperator(client)) {
		_server->queueMessage(client->getFd(), errChanOpPrivsNeeded(client->getNickname(), params[0]));
		return;
	}

	const std::string& mode = params[1];
	if (mode.size() < 2 || (mode[0] != '+' && mode[0] != '-')) {
		_server->queueMessage(client->getFd(), ":ircserv 472 " + client->getNickname() + " " + mode + " :is unknown mode char to me\r\n");
		return;
	}

	bool set = mode[0] == '+';
	char flag = mode[1];
	if (flag == 'i')
		channel->setInviteOnly(set);
	else if (flag == 't')
		channel->setTopicRestricted(set);
	else if (flag == 'k') {
		if (set) {
			if (params.size() < 3) {
				_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "MODE"));
				return;
			}
			channel->setKey(params[2]);
		} else {
			channel->setKey("");
		}
	} else if (flag == 'o') {
		if (params.size() < 3) {
			_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "MODE"));
			return;
		}
		Client* target = _server->getClientByNickname(params[2]);
		if (target == NULL || !channel->hasClient(target)) {
			_server->queueMessage(client->getFd(), ":ircserv 441 " + client->getNickname() + " " + params[2] + " " + params[0] + " :They aren't on that channel\r\n");
			return;
		}
		if (set)
			channel->addOperator(target);
		else
			channel->removeOperator(target);
	} else if (flag == 'l') {
		if (set) {
			if (params.size() < 3) {
				_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "MODE"));
				return;
			}
			int limit = std::atoi(params[2].c_str());
			if (limit <= 0) {
				_server->queueMessage(client->getFd(), ":ircserv 696 " + client->getNickname() + " " + params[0] + " l " + params[2] + " :Invalid limit\r\n");
				return;
			}
			channel->setUserLimit(limit);
		} else {
			channel->setUserLimit(0);
		}
	} else {
		_server->queueMessage(client->getFd(), ":ircserv 472 " + client->getNickname() + " " + std::string(1, flag) + " :is unknown mode char to me\r\n");
		return;
	}

	std::string modeMsg = ":" + clientMask(client) + " MODE " + params[0] + " " + mode;
	if ((flag == 'k' || flag == 'o' || (flag == 'l' && set)) && params.size() > 2)
		modeMsg += " " + params[2];
	modeMsg += "\r\n";
	channel->broadcastMessage(modeMsg, NULL);
	_server->queueMessage(client->getFd(), modeMsg);
}
