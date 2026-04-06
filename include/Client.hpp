#pragma once

class Client {

	public:
		Client() = default;
		~Client();

	private:
		int	_fd{-1};
};
