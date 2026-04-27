#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>

class Client;

class Channel {

	public:
		Channel(const std::string& name, Client* creator);
		~Channel();

		const std::string& getName() const;
		const std::string& getTopic() const;
		void setTopic(std::string topic);
		bool isInviteOnly() const;
		void setInviteOnly(bool inviteOnly);
		bool hasKey() const;
		bool checkKey(const std::string& key) const;
		void setKey(std::string key);
		int getUserLimit() const;
		void setUserLimit(int limit);
		void addClient(Client* client);
		void removeClient(Client* client);
		bool isOperator(Client* client) const;
		void addOperator(Client* client);
		void removeOperator(Client* client);
		void broadcastMessage(const std::string& msg, Client* sender);
		std::vector<Client*> getClients() const;

	private:
		std::string _name;
		std::string _topic;
		bool _inviteOnly;
		std::string _key;
		int _userLimit;
		std::vector<Client*> _clients;
		std::vector<Client*> _operators;
};

#endif
