#include "CommandParser.hpp"
#include "Utils.hpp"

CommandParser::ParsedCommand CommandParser::parse(const std::string& msg)
{
	ParsedCommand cmd;
	std::string line = Utils::trim(msg);

	if (line.empty())
		return cmd;

	// Command
	size_t pos = line.find(' ');
	// no space found
	if (pos == std::string::npos) {
		cmd.command = line;
		return cmd;
	}
	// otherwise extract substring before first space encountered as command
	cmd.command = line.substr(0, pos);

	// Params
	// break the loop when there are only spaces left in the unprocessed message
	while ((pos = line.find_first_not_of(' ', pos)) != std::string::npos) {

		// if the next param starts with ':', the rest of the message is the trailing
		if (line[pos] == ':') {
			cmd.trailing = line.substr(pos + 1);
			return cmd;
		}
		// find next param before space or line end
		size_t end = line.find(' ', pos);
		if (end == std::string::npos) {
			cmd.params.push_back(line.substr(pos));
			return cmd;
		}
		cmd.params.push_back(line.substr(pos, end - pos));
		pos = end;
	}
	return cmd;
}

bool CommandParser::validateCommand(const ParsedCommand& cmd)
{
	return !cmd.command.empty();
}
