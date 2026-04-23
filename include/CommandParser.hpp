#ifndef COMMAND_PARSER_HPP
#define COMMAND_PARSER_HPP

#include <string>
#include <vector>

class CommandParser {
public:
	struct ParsedCommand {
		std::string command;
		std::vector<std::string> params;
		std::string trailing;
	};
	static ParsedCommand parse(const std::string& message);
	static bool validateCommand(const ParsedCommand& cmd);
};

#endif
