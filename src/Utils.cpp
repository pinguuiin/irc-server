#include "Utils.hpp"
#include <cctype>
#include <iostream>

std::string Utils::toUpper(const std::string& str)
{
	std::string out;
	for (size_t i = 0; i < str.length(); ++i)
		out += static_cast<char>(std::toupper(static_cast<unsigned char>(str[i])));
	return out;
}

std::vector<std::string> Utils::split(const std::string& str, char delim)
{
	std::vector<std::string> parts;
	std::string cur;
	for (size_t i = 0; i < str.length(); ++i) {
		if (str[i] == delim) {
			parts.push_back(cur);
			cur.clear();
		} else {
			cur += str[i];
		}
	}
	parts.push_back(cur);
	return parts;
}

std::string Utils::trim(const std::string& str)
{
	size_t start = 0;
	while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start])))
		++start;
	size_t end = str.length();
	while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
		--end;
	return str.substr(start, end - start);
}

bool Utils::isValidNickname(const std::string& nick)
{
	(void)nick;
	std::cerr << "TODO: Utils::isValidNickname" << std::endl;
	return true;
}

bool Utils::isValidChannelName(const std::string& name)
{
	(void)name;
	std::cerr << "TODO: Utils::isValidChannelName" << std::endl;
	return true;
}
