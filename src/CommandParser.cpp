#include "CommandParser.hpp"
#include "Utils.hpp"

CommandParser::ParsedCommand CommandParser::parse(const std::string& msg)
{
	ParsedCommand cmd;
	std::string line = Utils::trim(msg);

	if (line.empty())
		return cmd;

	// Command
	auto pos = msg.find(' ');
	// no space found
	if (pos == std::string::npos) {
		cmd.command = msg;
		return cmd;
	}
	// otherwise extract substring before first space encountered as command
	cmd.command = msg.substr(0, pos);

	// Params
	// break the loop when there are only spaces left in the unprocessed message
	while ((pos = msg.find_first_not_of(' ', pos)) != std::string::npos) {
		// find next param before space or line end
		auto end = msg.find(' ', pos);
		if (end == std::string::npos) {
			cmd.params.push_back(msg.substr(pos));
			return cmd;
		}
		cmd.params.push_back(msg.substr(pos, end - pos));
		pos = end;
	}
	return cmd;
}

bool CommandParser::validateCommand(const ParsedCommand& cmd)
{
	return !cmd.command.empty();
}
