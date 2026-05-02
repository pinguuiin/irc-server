#include "CommandParser.hpp"
#include "Utils.hpp"

CommandParser::ParsedCommand CommandParser::parse(const std::string& message)
{
	ParsedCommand cmd;
	std::string line = Utils::trim(message);
	if (line.empty())
		return cmd;

	size_t trailingPos = line.find(" :");
	if (trailingPos != std::string::npos) {
		cmd.trailing = line.substr(trailingPos + 2);
		line = line.substr(0, trailingPos);
	}

	std::vector<std::string> tokens = Utils::split(line, ' ');
	for (size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].empty())
			continue;
		if (cmd.command.empty())
			cmd.command = tokens[i];
		else
			cmd.params.push_back(tokens[i]);
	}
	return cmd;
}

bool CommandParser::validateCommand(const ParsedCommand& cmd)
{
	return !cmd.command.empty();
}
