#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include "CommandParser.hpp"
#include <string>
#include <vector>

class Server;
class Client;

class CommandHandler {

	public:
		CommandHandler(Server* server);
		void handleCommand(Client* client, const CommandParser::ParsedCommand& cmd);

	private:
		Server* _server;
		void handlePass(Client* client, const std::vector<std::string>& params);
		void handleNick(Client* client, const std::vector<std::string>& params);
		void handleUser(Client* client, const std::vector<std::string>& params);
		void handleJoin(Client* client, const std::vector<std::string>& params);
		void handlePrivmsg(Client* client, const std::vector<std::string>& params);
		void handleKick(Client* client, const std::vector<std::string>& params);
		void handleInvite(Client* client, const std::vector<std::string>& params);
		void handleTopic(Client* client, const std::vector<std::string>& params);
		void handleMode(Client* client, const std::vector<std::string>& params);
};

#endif
