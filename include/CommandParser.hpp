/**
 * @file CommandParser.hpp
 * @brief Declaration of the CommandParser class.
 *
 * CommandParser is a stateless utility class that parses raw IRC message
 * strings into a structured ParsedCommand. It follows the IRC message format
 * from RFC 1459:
 *
 *   [:<prefix> ] <command> [<params>...] [:<trailing>]\r\n
 *
 * The optional ':' prefix sent by some clients is silently skipped because
 * the server always derives sender identity from the Client object, not the
 * message prefix.
 */

#ifndef COMMAND_PARSER_HPP
#define COMMAND_PARSER_HPP

#include <string>
#include <vector>

/**
 * @class CommandParser
 * @brief Parses raw IRC message lines into a structured command representation.
 *
 * All methods are static — CommandParser has no instance state and does not
 * need to be instantiated.
 *
 * Example:
 * @code
 *   std::string raw = "JOIN #general secretkey";
 *   CommandParser::ParsedCommand cmd = CommandParser::parse(raw);
 *   // cmd.command  == "JOIN"
 *   // cmd.params   == { "#general", "secretkey" }
 *   // cmd.trailing == ""
 * @endcode
 */
class CommandParser {

	public:
		/**
		 * @struct ParsedCommand
		 * @brief Holds the result of parsing a single IRC message line.
		 *
		 * Fields:
		 *  - command:  The IRC verb in its original casing (e.g. "JOIN").
		 *              CommandHandler::handleCommand() uppercases it before dispatch.
		 *  - params:   Space-separated parameters that appear before any
		 *              trailing parameter.
		 *  - trailing: The text after the first ' :' separator (may be empty).
		 *              CommandHandler prepends it to params before calling handlers.
		 */
		struct ParsedCommand {
			std::string command;				//< IRC command verb (e.g. "PRIVMSG")
			std::vector<std::string> params;	//< Space-delimited parameters
			std::string trailing;				//< Text after ' :' separator (may be empty)
		};

		/**
		 * @brief Parses a single raw IRC message line into a ParsedCommand.
		 *
		 * Leading whitespace is trimmed. A leading ':' prefix line is skipped.
		 * Parsing stops at the first ' :' token; everything after it becomes
		 * ParsedCommand::trailing.
		 *
		 * @param msg A single IRC message line without its \\r\\n ending.
		 * @return A ParsedCommand. If @p msg is empty or malformed, the returned
		 *         struct has an empty command field.
		 */
		static ParsedCommand parse(const std::string& msg);

		/**
		 * @brief Returns true if the ParsedCommand has a non-empty command field.
		 *
		 * Used by Client::receiveAndHandleMessage() to silently discard blank
		 * or unparseable lines before dispatch.
		 *
		 * @param cmd The ParsedCommand to validate.
		 */
		static bool validateCommand(const ParsedCommand& cmd);
};

#endif
