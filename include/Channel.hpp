/**
 * @file Channel.hpp
 * @brief Declaration of the Channel class representing an IRC channel.
 *
 * A Channel holds the set of connected clients, its operator list,
 * its invite list, and all channel-mode flags (+i, +t, +k, +l).
 * It does NOT manage its own lifetime — the Server creates and destroys
 * Channel instances through createChannel() / removeChannel().
 */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>

class Client;
class Server;

/**
 * @class Channel
 * @brief Represents a single IRC channel and all of its state.
 *
 * Responsibilities:
 *  - Tracking which clients are members, operators, or on the invite list.
 *  - Enforcing channel modes: invite-only (+i), topic-restricted (+t),
 *    key-protected (+k), and user-limit (+l).
 *  - Broadcasting messages to all members (optionally excluding the sender).
 *
 * The Channel holds a raw pointer back to the owning Server so it can call
 * Server::queueMessage() inside broadcastMessage(). The pointer is never
 * owned by Channel — Server is responsible for its own lifetime.
 */
class Channel {

	public:
		/**
		 * @brief Constructs a new Channel.
		 *
		 * Initialises all mode flags to their defaults:
		 *  - invite-only:       false
		 *  - key:               empty string (no key)
		 *  - topic-restricted:  true  (only operators may set the topic)
		 *  - user limit:        0 (no limit)
		 *
		 * @param name   The channel name (e.g. "#general").
		 * @param server Pointer to the owning Server instance.
		 */
		Channel(const std::string& name, Server* server);

		/**
		 * @brief Destructor. Does not close any file descriptors or free clients.
		 */
		~Channel();

	// ── Getters ────────────────────────────────────────────────────────
		/** @brief Returns the channel name (e.g. "#general"). */
		const std::string& getName() const;

		/** @brief Returns the current topic (empty if none is set). */
		const std::string& getTopic() const;

		/** @brief Returns the channel key/password (empty if none). */
		const std::string& getKey() const;

		/** @brief Sets the channel topic. @param topic New topic string. */
		void setTopic(std::string topic);

		/** @brief Returns true if invite-only mode (+i) is active. */
		bool isInviteOnly() const;

		/** @brief Enables or disables invite-only mode (+i). */
		void setInviteOnly(bool inviteOnly);

		 /** @brief Returns true if a key is required to join (+k). */
		bool hasKey() const;

		/**
		 * @brief Validates a join key provided by a client.
		 *
		 * Returns true unconditionally if no key is set.
		 *
		 * @param key Key string supplied by the joining client.
		 * @return true if the key matches or the channel has no key.
		 */
		bool checkKey(const std::string& key) const;

		/**
		 * @brief Sets or clears the channel key.
		 *
		 * Pass an empty string to remove the key (equivalent to MODE -k).
		 *
		 * @param key The new key, or "" to remove it.
		 */
		void setKey(std::string key);

		 /** @brief Returns true if only operators may change the topic (+t). */
		bool isTopicRestricted() const;

		/** @brief Enables or disables topic restriction (+t). */
		void setTopicRestricted(bool restricted);

		/**
		 * @brief Returns the maximum number of members allowed (+l).
		 * @return The user limit, or 0 if unlimited.
		 */
		int getUserLimit() const;

		/**
		 * @brief Sets the maximum number of members (+l).
		 *
		 * Pass 0 to remove the limit (equivalent to MODE -l).
		 *
		 * @param limit New user limit (0 = unlimited).
		 */
		void setUserLimit(int limit);

	// ── Member management ──────────────────────────────────────────────
		/**
		 * @brief Adds a client to the channel member list.
		 *
		 * Does nothing if the client is NULL, already a member,
		 * or if the user limit would be exceeded.
		 * Clears any pending invite for the client on successful join.
		 *
		 * @param client Pointer to the joining Client.
		 */
		void addClient(Client* client);

		/**
		 * @brief Removes a client from the member, operator, and invite lists.
		 *
		 * Does nothing if client is NULL.
		 *
		 * @param client Pointer to the departing Client.
		 */
		void removeClient(Client* client);

		/**
		 * @brief Returns true if the given client is a channel member.
		 * @param client Pointer to the Client to check.
		 */
		bool hasClient(Client* client) const;

	// ── Operator management ────────────────────────────────────────────
		/**
		 * @brief Returns true if the given client has operator status.
		 * @param client Pointer to the Client to check.
		 */
		bool isOperator(Client* client) const;

		/**
		 * @brief Grants operator status to a channel member.
		 *
		 * Does nothing if client is NULL, not a member, or already an operator.
		 *
		 * @param client Pointer to the Client to promote.
		 */
		void addOperator(Client* client);

		/**
		 * @brief Revokes operator status from a client.
		 *
		 * Does nothing if client is NULL or not currently an operator.
		 *
		 * @param client Pointer to the Client to demote.
		 */
		void removeOperator(Client* client);

	// ── Invite management ──────────────────────────────────────────────
		/**
		 * @brief Adds a client to the invite list.
		 *
		 * Does nothing if client is NULL or already invited.
		 *
		 * @param client Pointer to the invited Client.
		 */
		void inviteClient(Client* client);

		/**
		 * @brief Removes a client from the invite list.
		 *
		 * Called automatically by addClient() after a successful join.
		 *
		 * @param client Pointer to the Client whose invite should be cleared.
		 */
		bool isInvited(Client* client) const;

		/**
		 * @brief Removes a client from the invite list.
		 *
		 * Called automatically by addClient() after a successful join.
		 *
		 * @param client Pointer to the Client whose invite should be cleared.
		 */
		void clearInvite(Client* client);

	// ── Messaging ──────────────────────────────────────────────────────
		/**
		 * @brief Sends a message to every member except the sender.
		 *
		 * Uses Server::queueMessage() so messages go through the normal
		 * non-blocking send pipeline.
		 *
		 * @param msg    Fully-formed IRC message string (including \\r\\n).
		 * @param sender Client to exclude from the broadcast, or NULL to
		 *               send to all members.
		 */
		void broadcastMessage(const std::string& msg, Client* sender);

		/**
		 * @brief Returns a copy of the current member list.
		 * @return Vector of raw (non-owning) Client pointers.
		 */
		std::vector<Client*> getClients() const;

	private:
		std::string _name;					//< Channel name (e.g. "#general")
		std::string _topic;					//< Current topic (empty = no topic set)
		bool _inviteOnly;					//< Mode +i: only invited users may join
		std::string _key;					//< Mode +k: join password (empty = none)
		bool _topicRestricted;				//< Mode +t: only operators may set topic
		int _userLimit;						//< Mode +l: max members (0 = unlimited)
		std::vector<Client*> _clients;		//< Current channel members
		std::vector<Client*> _operators;	//< Members with operator (+o) status
		std::vector<Client*> _invited;		//< Clients with a pending INVITE
		Server* _server;					//< Back-pointer to the owning Server (not owned)
};

#endif
