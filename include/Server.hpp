#pragma once

class Server {

	public:
		Server() = default;
		~Server();

		void createSocket();
		void initServer();

	private:
		static constexpr uint16_t _port = 9999;
		int _sockFd{-1}; // listening socket file descriptor
};
