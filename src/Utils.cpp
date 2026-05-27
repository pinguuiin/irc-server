#include "Utils.hpp"
#include <cctype>

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

/**
 * Validates an IRC nickname against RFC 1459 §2.3.1:
 *   - Length: 1–9 characters.
 *   - First character: letter or special ([]\\^_`{|}).
 *   - Remaining characters: letter, digit, hyphen, or special.
 */
bool Utils::isValidNickname(const std::string& nick)
{
	// Rule 1: length between 1 and 9
	if (nick.empty() || nick.size() > 9)
		return false;

	// Helper: the "special" characters the RFC allows in nicks
	auto isSpecial = [](char c) -> bool {
		// These are the exact characters listed in RFC 1459 §2.3.1
		return std::string("[]\\^_`{|}").find(c) != std::string::npos;
	};

	// Rule 2: first character must be a letter or special
	if (!std::isalpha(static_cast<unsigned char>(nick[0])) && !isSpecial(nick[0]))
		return false;

	// Rule 3: remaining characters must be letter, digit, '-', or special
	for (size_t i = 1; i < nick.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(nick[i]);
		if (!std::isalnum(c) && !isSpecial(nick[i]) && nick[i] != '-')
			return false;
	}

	return true;
}

/**
 * Validates an IRC channel name against RFC 1459 §1.3:
 *   - Must start with '#'.
 *   - Length: 2–50 characters (including the '#').
 *   - Must not contain: space ' ', bell '\a', or comma ','.
 *
 * Examples:
 *   "#general"  → valid
 *   "#a"        → valid  (just '#' + one char)
 *   "#"         → INVALID (no name after the '#')
 *   "general"   → INVALID (missing '#' prefix)
 *   "(#bad name)" → INVALID (contains space)
 */
bool Utils::isValidChannelName(const std::string& name)
{
	// Rule 1: must start with '#'
	if (name.empty() || name[0] != '#')
		return false;

	// Rule 2: must have at least one character after '#'
	if (name.size() < 2)
		return false;

	// Rule 3: maximum 50 characters total
	if (name.size() > 50)
		return false;

	// Rule 4: no spaces, no bell character (\a), no commas
	for (size_t i = 1; i < name.size(); ++i)
	{
		char c = name[i];
		if (c == ' ' || c == '\a' || c == ',')
			return false;
	}
	return true;
}
