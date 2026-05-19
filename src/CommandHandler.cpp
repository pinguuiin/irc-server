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
	else if (command == "CAP")
		handleCap(client, params);
	else if (command == "PART")
		handlePart(client, params);
	else if (command == "PING")
		handlePing(client, params);
	else if (command == "QUIT")
		handleQuit(client, params);
	else
		_server->queueMessage(client->getFd(), errUnknownCommand(client->getNickname(), command));
}
// ── handleCap ───────────────────────────────────────────────────────
// irssi sends "CAP LS" before NICK/USER to negotiate extra features.
// We don't support any capabilities, so we reply with an empty list
// and then send "CAP END" to tell the client to stop waiting and
// proceed with normal registration (NICK / USER / PASS).
void CommandHandler::handleCap(Client* client, const std::vector<std::string>& params)
{
	if (params.empty())
		return;

	std::string subcommand = Utils::toUpper(params[0]);
	if(subcommand == "LS")
	{
		// Empty capability list - we support nothing extra
		_server->queueMessage(client->getFd(), ":ircserv CAP * LS :\r\n");
	}
	else if (subcommand == "REQ")
	{
		// Reject any capability the client requests
		_server->queueMessage(client->getFd(), ":ircserv CAP * NAK :" + (params.size() > 1 ? params[1] : "")
			+ "\r\n");
	}
	else if (subcommand == "END")
	{
		// Client is done with capability negotiaton, do nothing
		// tryCompleteRegistration will fire once PASS+NICK+USER arrive
	}

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
	// 462 ERR_ALREADYREGISTRED - USER cannot be sent more than once per session.
	// Once the client is authenticated (001 was sent), reject any new USER command(guard).
	if (client->isAuthenticated())
	{
		_server->queueMessage(client->getFd(), ":ircserv 462 " + client->getNickname() + " :You may not reregister\r\n");
		return;
	}
	if (params.empty())
	{
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
	if (client->isAuthenticated()) // fast exit for already-registered clients
		return;
	if (!client->isPassOk() || !client->isNickSet() || !client->isUserSet())
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

	// Build the names list: prefix '@' for operators, nothing for regular users
	std::string namesList;
	const std::vector<Client*>& members = channel->getClients();

	for (size_t i = 0; i < members.size(); ++i)
	{
		// Add space separator between names (but not before the first one)
		if (i != 0)
			namesList += " ";

		// '@' prefix if this member is a channel operator
		if (channel->isOperator(members[i]))
			namesList += "@";

		namesList += members[i]->getNickname();
	}
	// TOPIC (332 or 331; always send one) - send the existing topic (if there is one) to the joining client
	// irssi displays this in the channel header
	if (!channel->getTopic().empty())
	{
		_server->queueMessage(client->getFd(), ":ircserv 332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
	}
	else
		_server->queueMessage(client->getFd(), ":ircserv 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n");

	// 353 RPL_NAMREPLY - "=" means public channel (vs "@" secret or "*" private)
	// Format: :server 353 <yournick> = <#channel> : <names...>
	_server->queueMessage(client->getFd(), ":ircserv 353 " + client->getNickname() +  " = " + channelName + " :" + namesList + "\r\n");

	// 366 RPL_ENDOFNAME - tells irssi the names list is complete
	// Format: :server 366 <yournick> <#channel> :End of /NAME list
	_server->queueMessage(client->getFd(), ":ircserv 366 " + client->getNickname() + " " + channelName + " :End of /NAMES list\r\n");

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
	channel->broadcastMessage(kickMsg, target); // exclude target from broadcast
	_server->queueMessage(target->getFd(), kickMsg); // send once to target
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
	channel->broadcastMessage(":" + clientMask(client) + " TOPIC " + params[0] + " :" + params[1] + "\r\n", client); // <-- pass client, not NULL, to exclude them from broadcast
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
	channel->broadcastMessage(modeMsg, client); // <-- pass client, not NULL, to exclude them from broadcast
	_server->queueMessage(client->getFd(), modeMsg);
}

// ── handlePart ──────────────────────────────────────────────────────
// Removes the client from a channel.
// Broadcasts PART to all members (including the departing client)
// before removing them from the member list.
void CommandHandler::handlePart(Client* client, const std::vector<std::string>& params)
{
	if(params.empty())
	{
		_server->queueMessage(client->getFd(), errNeedMoreParams(client->getNickname(), "PART"));
		return;
	}
	Channel* channel = _server->getChannel(params[0]);
	if (channel == NULL)
	{
		_server->queueMessage(client->getFd(), errNoSuchChannel(client->getNickname(), params[0]));
		return;
	}
	if (!channel->hasClient(client))
	{
		_server->queueMessage(client->getFd(), errNotOnChannel(client->getNickname(), params[0]));
		return;
	}
	// Optional reason is the trailing parameter (params[1] if present)
	std::string reason = params.size() > 1 ? params[1] : client->getNickname();
	std::string partMsg = ":" + clientMask(client) + " PART " + params[0] + " :" + reason + "\r\n";

	// Broadcast to everyone EXCEPT the leaving client, then send directly to them
	channel->broadcastMessage(partMsg, client);
	_server->queueMessage(client->getFd(), partMsg);
	channel->removeClient(client);
}

// ── handlePing ──────────────────────────────────────────────────────
// Responds to PING with PONG.
// IRC clients send PING regularly; no reply = disconnect after timeout.
// Format:  PING <token>  →  :ircserv PONG ircserv :<token>
void CommandHandler::handlePing(Client* client, const std::vector<std::string>& params)
{
	std::string token = params.empty() ? "ircserv" : params[0];
	_server->queueMessage(client->getFd(), ":ircserv PONG ircserv :" + token + "\r\n");
}

// ── handleQuit ──────────────────────────────────────────────────────
// Handles a graceful disconnect.
// 1. Broadcasts QUIT to every channel the client is in.
// 2. Removes the client from each channel's member list.
// 3. Tells the server to close the fd and delete the client object.
void CommandHandler::handleQuit(Client* client, const std::vector<std::string>& params)
{
	std::string reason = params.empty() ? "Client quit" : params[0];
	std::string quitMsg = ":" + clientMask(client) + " QUIT :" + reason + "\r\n";

	const std::map<std::string, Channel*>& channels = _server->getChannels();
	std::vector<Channel*> toLeave;

	for (std::map<std::string, Channel*>::const_iterator it = channels.begin(); it != channels.end(); ++it)
	{
		if (it->second->hasClient(client))
			toLeave.push_back(it->second);
	}
	for (size_t i = 0; i < toLeave.size(); ++i)
	{
		toLeave[i]->broadcastMessage(quitMsg, client); // notify others, not the quitter
		toLeave[i]->removeClient(client);
	}
	_server->removeClient(client->getFd());
}
