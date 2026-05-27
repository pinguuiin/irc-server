/**
 * @file CommandHandler.hpp
 * @brief Declaration of the CommandHandler class.
 *
 * CommandHandler is the central dispatcher for all IRC commands received
 * from clients. It receives a fully parsed command (from CommandParser),
 * checks authentication state, and routes the call to the appropriate
 * private handler method.
 *
 * Supported commands
 * ------------------
 *   Pre-auth  : PASS, NICK, USER, CAP, PING, QUIT
 *   Post-auth : JOIN, PRIVMSG, KICK, INVITE, TOPIC, MODE, PART
 */

#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include "CommandParser.hpp"
#include <string>
#include <vector>

class Server;
class Client;

/**
 * @class CommandHandler
 * @brief Routes and processes IRC commands on behalf of a connected client.
 *
 * Each call to handleCommand() is stateless with respect to the handler
 * itself — all persistent state lives in the Client, Channel, and Server
 * objects. CommandHandler holds only a pointer to the Server so it can
 * access channels, clients, and the message queue.
 *
 * All private handlers follow the same pattern:
 *  1. Validate parameter count   → send 461 ERR_NEEDMOREPARAMS if missing.
 *  2. Validate arguments         → nick validity, channel existence, etc.
 *  3. Apply the change.
 *  4. Broadcast or reply with the appropriate IRC numeric / message.
 */
class CommandHandler {

	public:
		/**
		 * @brief Constructs a CommandHandler bound to the given Server.
		 * @param server Non-owning pointer to the Server instance.
		 */
		CommandHandler(Server* server);

		/**
		 * @brief Dispatches an IRC command to the correct handler method.
		 *
		 * The command name is uppercased for case-insensitive matching.
		 * Pre-auth commands (PASS, NICK, USER, CAP, PING, QUIT) are always
		 * allowed. All other commands require a fully registered client;
		 * unregistered clients receive 451 ERR_NOTREGISTERED and the call
		 * returns immediately.
		 *
		 * The trailing parameter (if any) is appended to the params vector
		 * before dispatch so individual handlers do not need to handle it
		 * separately.
		 *
		 * @param client Pointer to the Client that sent the command.
		 * @param cmd    Parsed command struct from CommandParser::parse().
		 */
		void handleCommand(Client* client, const CommandParser::ParsedCommand& cmd);

	private:
		Server* _server;		//< Non-owning pointer to the owning Server

	// ── Command handlers ───────────────────────────────────────────────
	// Each handler corresponds to exactly one IRC command.
	// All share the same signature: the sending client and the parameter
	// list (with the trailing parameter already appended by handleCommand).

		/**
		 * @brief PASS — validates the server password.
		 *
		 * Marks _passOk on the client if the password matches, then calls
		 * tryCompleteRegistration(). Sends 464 ERR_PASSWDMISMATCH on failure.
		 */
		void handlePass(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief NICK — sets or changes a client's nickname.
		 *
		 * During registration: validates format, checks uniqueness
		 * (433 ERR_NICKNAMEINUSE), stores the nick, marks _nickSet, and
		 * calls tryCompleteRegistration().
		 * After registration: broadcasts a NICK message to all channels the
		 * client is currently in.
		 */
		void handleNick(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief USER — sets the client's username (ident).
		 *
		 * Can only be sent once per session. Sends 462 ERR_ALREADYREGISTRED
		 * if the client is already authenticated. Marks _userSet and calls
		 * tryCompleteRegistration() on success.
		 */
		void handleUser(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief JOIN — adds a client to a channel.
		 *
		 * Creates the channel if it does not exist. The first member
		 * automatically receives operator status. Enforces +i, +k, and +l.
		 * Sends JOIN broadcast, topic reply (332/331), and name list (353/366)
		 * to the joining client.
		 */
		void handleJoin(Client* client, const std::vector<std::string>& params);

		 /**
		 * @brief PRIVMSG — sends a message to a channel or a user.
		 *
		 * Supports both channel targets ("#general") and direct nick-to-nick
		 * messages. Sends 401 ERR_NOSUCHNICK or 403 ERR_NOSUCHCHANNEL if the
		 * target does not exist.
		 */
		void handlePrivmsg(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief KICK — forcibly removes a user from a channel.
		 *
		 * Requires channel-operator status. Broadcasts the KICK message to
		 * all members before removing the target.
		 * Sends 482 ERR_CHANOPRIVSNEEDED if the caller is not an operator.
		 */
		void handleKick(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief INVITE — invites a user to a channel.
		 *
		 * The caller must be a channel member (and an operator if the channel
		 * is +i). Adds the target to the channel's invite list and notifies
		 * both the caller (341 RPL_INVITING) and the target.
		 */
		void handleInvite(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief TOPIC — queries or sets the channel topic.
		 *
		 * One parameter: returns 332 RPL_TOPIC or 331 RPL_NOTOPIC.
		 * Two parameters: sets the topic (respecting +t), broadcasts the
		 * change to all members.
		 */
		void handleTopic(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief MODE — queries or modifies channel modes.
		 *
		 * Supported flags: +i, +t, +k, +l, +o.
		 * No mode string: returns current modes via 324 RPL_CHANNELMODEIS.
		 * Requires operator status to change modes. Broadcasts a MODE message
		 * to all members (including the caller) when changes are applied.
		 */
		void handleMode(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief CAP — IRC capability negotiation.
		 *
		 * This server supports no extended capabilities.
		 * CAP LS  → replies with an empty capability list.
		 * CAP REQ → replies with NAK for all requests.
		 * CAP END → silently accepted; irssi then proceeds with registration.
		 */
		void handleCap(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief PART — removes the calling client from a channel.
		 *
		 * Broadcasts a PART message to all remaining members. Destroys the
		 * channel via Server::removeChannel() if it becomes empty after the
		 * client leaves.
		 */
		void handlePart(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief PING — responds to a keepalive ping with a matching PONG.
		 *
		 * IRC clients send PING periodically; no reply within the client's
		 * timeout causes a disconnect.
		 * Format: PING <token> → :ircserv PONG ircserv :<token>
		 */
		void handlePing(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief QUIT — performs a graceful client disconnect.
		 *
		 * Broadcasts a QUIT message to every channel the client is in,
		 * removes the client from each channel (destroying empty ones),
		 * then calls Server::removeClient() to close the file descriptor.
		 */
		void handleQuit(Client* client, const std::vector<std::string>& params);

		/**
		 * @brief Attempts to complete client registration after PASS/NICK/USER.
		 *
		 * Called at the end of handlePass(), handleNick(), and handleUser().
		 * If all three flags are set, calls Client::authenticate() and sends
		 * the welcome burst: 001 RPL_WELCOME, 002 RPL_YOURHOST,
		 * 003 RPL_CREATED, 004 RPL_MYINFO.
		 * irssi waits for 001 before considering itself connected.
		 */
		void tryCompleteRegistration(Client* client);
};

#endif
