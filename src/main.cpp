#include "../include/Server.hpp"

#include <iostream>

int stringToPort(const char *s)
{
	int num;

	for (int i = 0; s[i]; ++i) {
		if (!std::isdigit(s[i]))
			return -1;
	}
	num = std::atoi(s);
	if (long long ll{std::atoll(s)}; num != ll)
		return -1;
	if (num >= 1024 && num <= 65535)
		return num;
	return -1;
}

int main(int argc, char *argv[])
{
	int num;

	if (argc != 3) {
		std::cerr << "Invalid input. Only 2 arguments are allowed" << std::endl;
		return 1;
	}
	num = stringToPort(argv[1]);
	if (num == -1) {
		std::cerr << "Invalid port. Available range 1024 - 65535" << std::endl;
		return 1;
	}

	std::cout << "\n==================== IRC SERVER ====================\n" << std::endl;

	Server server(num, argv[2]);

	try {
		server.initServer();

		/* Anything else */

	}
	catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
