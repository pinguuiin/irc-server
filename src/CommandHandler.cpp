#include "CommandHandler.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include <iostream>

CommandHandler::CommandHandler(Server* server)
	: _server(server)
{
	std::cerr << "TODO: CommandHandler::CommandHandler" << std::endl;
}

void CommandHandler::handleCommand(Client* client, const CommandParser::ParsedCommand& cmd)
{
	(void)client;
	(void)cmd;
	std::cerr << "TODO: CommandHandler::handleCommand (server=" << _server << ")" << std::endl;
}

void CommandHandler::handlePass(Client* client, const std::vector<std::string>& params)
{
	(void)client;
	(void)params;
	std::cerr << "TODO: CommandHandler::handlePass" << std::endl;
}

void CommandHandler::handleNick(Client* client, const std::vector<std::string>& params)
{
	(void)client;
	(void)params;
	std::cerr << "TODO: CommandHandler::handleNick" << std::endl;
}

void CommandHandler::handleUser(Client* client, const std::vector<std::string>& params)
{
	(void)client;
	(void)params;
	std::cerr << "TODO: CommandHandler::handleUser" << std::endl;
}

void CommandHandler::handleJoin(Client* client, const std::vector<std::string>& params)
{
	(void)client;
	(void)params;
	std::cerr << "TODO: CommandHandler::handleJoin" << std::endl;
}

void CommandHandler::handlePrivmsg(Client* client, const std::vector<std::string>& params)
{
	(void)client;
	(void)params;
	std::cerr << "TODO: CommandHandler::handlePrivmsg" << std::endl;
}

void CommandHandler::handleKick(Client* client, const std::vector<std::string>& params)
{
	(void)client;
	(void)params;
	std::cerr << "TODO: CommandHandler::handleKick" << std::endl;
}

void CommandHandler::handleInvite(Client* client, const std::vector<std::string>& params)
{
	(void)client;
	(void)params;
	std::cerr << "TODO: CommandHandler::handleInvite" << std::endl;
}

void CommandHandler::handleTopic(Client* client, const std::vector<std::string>& params)
{
	(void)client;
	(void)params;
	std::cerr << "TODO: CommandHandler::handleTopic" << std::endl;
}

void CommandHandler::handleMode(Client* client, const std::vector<std::string>& params)
{
	(void)client;
	(void)params;
	std::cerr << "TODO: CommandHandler::handleMode" << std::endl;
}
