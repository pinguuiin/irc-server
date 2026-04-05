#pragma once

class Client {

	public:
		Client() = default;
		~Client();

	private:
		int	_sockFd{-1};
};
