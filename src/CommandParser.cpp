#include "CommandParser.hpp"
#include <iostream>

CommandParser::ParsedCommand CommandParser::parse(const std::string& message)
{
	(void)message;
	std::cerr << "TODO: CommandParser::parse" << std::endl;
	ParsedCommand cmd;
	return cmd;
}

bool CommandParser::validateCommand(const ParsedCommand& cmd)
{
	(void)cmd;
	std::cerr << "TODO: CommandParser::validateCommand" << std::endl;
	return false;
}
