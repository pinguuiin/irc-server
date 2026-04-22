#pragma once

#include <string>

class Client {

	public:
		Client() = default;
		~Client() = default;

		// --- Ping's original -----
		const int			&getFd() const;
		const std::string	&getIp() const;
		void 				setFd(int fd);
		void 				setIp(const std::string &ip);

		// --- Auth state (yours) ----
		bool	hasPassOk() const;		// did client send correct PASS?
		bool	isRegistered() const; 	// did client finish NICK + USER?
		bool	isOper() const;			// did client use OPER command?
		void	setPassOk(bool val);
		void	setRegistered(bool val);
		void	setOper(bool val);

		// --- Identity (yours) -----
		const	std::string &getNick() const;
		const	std::string &getUsername() const;
		const	std::string &getRealname() const;
		void				setNick(const std::string &nick);
		void				setUsername(const std::string &user);
		void				setRealname(const std::string &real);

		// --- Message buffer (yours) -----
		// TCP may deliver half a message - we collect bytes here
		// until we see \r\n, then process the complete line
		std::string &getMsgBuffer();

	private:
		// Ping's fields
		int			_fd{-1};
		std::string	_ip;

		// Daniel's fields
		bool	_passOk{false};
		bool	_registered{false};
		bool	_isOper{false};

		std::string	_nick;
		std::string	_username;
		std::string	_realname;
		std::string	_msgBuffer;

};
