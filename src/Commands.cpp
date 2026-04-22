#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <vector>
#include <string>

// Splits one IRC line into tokens.
// The word starting with ':' is a "trailing" parameter — it
// absorbs everything after it, including spaces.
static std::vector<std::string> tokenize(const std::string& line)
{
    std::vector<std::string> tokens;
    size_t i = 0;

    // Some clients send ":prefix COMMAND ..." — skip the prefix
    if (!line.empty() && line[0] == ':') {
        i = line.find(' ');
        if (i == std::string::npos) return tokens;
        ++i;
    }

    while (i < line.size()) {
        while (i < line.size() && line[i] == ' ') ++i; // skip spaces
        if (i >= line.size()) break;

        if (line[i] == ':') {           // trailing param — take rest of line
            tokens.push_back(line.substr(i + 1));
            break;
        }

        size_t start = i;
        while (i < line.size() && line[i] != ' ') ++i;
        tokens.push_back(line.substr(start, i - start));
    }
    return tokens;
}

// Adds \r\n and sends a message to one client.
// Every single IRC reply MUST end with \r\n — this handles that.
void Server::sendReply(int fd, const std::string& msg)
{
    std::string out = msg + "\r\n";
    send(fd, out.c_str(), out.size(), 0);
}

// Returns ":nick!user@ip" — the sender label that goes at the
// start of relayed IRC messages
std::string Server::getClientPrefix(int fd) const
{
    const auto& c = _client.at(fd);
    std::string nick = c.getNick().empty()     ? "*" : c.getNick();
    std::string user = c.getUsername().empty() ? "*" : c.getUsername();
    return ":" + nick + "!" + user + "@" + c.getIp();
}


// Cleanly removes a client from everything.
// If you skip _nickMap cleanup, that nickname is gone forever
// until the server restarts.
void Server::disconnectClient(int fd)
{
    auto it = _client.find(fd);
    if (it == _client.end()) return; // already removed

    // Remove nick from lookup map so others can take it
    if (!it->second.getNick().empty())
        _nickMap.erase(it->second.getNick());

    // Stop epoll from watching this fd
    epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr);

    _client.erase(it);
    close(fd);
    std::cout << "Client fd=" << fd << " disconnected\n";
}

// Called after every recv(). Extracts complete IRC lines
// (terminated by \r\n) and sends each one to parseAndDispatch().
// Incomplete lines stay in the buffer until the rest arrives.
void Server::processBuffer(int fd)
{
    std::string& buf = _client[fd].getMsgBuffer();
    size_t pos;

    while ((pos = buf.find('\n')) != std::string::npos)
	{
        std::string line = buf.substr(0, pos); // grab line without \r\n
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

        buf.erase(0, pos + 1);                 // remove it from buffer
        if (!line.empty())
            parseAndDispatch(fd, line);
    }
}

void Server::parseAndDispatch(int fd, const std::string& line)
{
    auto tokens = tokenize(line);
    if (tokens.empty()) return;

    // IRC commands are case-insensitive — normalise to uppercase
    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    tokens[0] = cmd;

    // These commands are allowed BEFORE the client registers
    if (cmd == "CAP")  { return; } // just ignore, keeps clients happy
    if (cmd == "PASS") { handlePass(fd, tokens); return; }
    if (cmd == "NICK") { handleNick(fd, tokens); return; }
    if (cmd == "USER") { handleUser(fd, tokens); return; }
    if (cmd == "QUIT") { disconnectClient(fd);   return; }
    if (cmd == "PING") {
        std::string token = tokens.size() > 1 ? tokens[1] : SERVER_NAME;
        sendReply(fd, ":" SERVER_NAME " PONG " SERVER_NAME " :" + token);
        return;
    }

    // Everything below requires the client to be registered first
    if (!_client[fd].isRegistered()) {
        std::string nick = _client[fd].getNick().empty() ? "*" : _client[fd].getNick();
        sendReply(fd, ":" SERVER_NAME " 451 " + nick + " :You have not registered");
        return;
    }

    // Your commands
    if (cmd == "PRIVMSG") { handlePrivmsg(fd, tokens); return; }
    if (cmd == "NOTICE")  { handlePrivmsg(fd, tokens); return; } // same logic
    if (cmd == "OPER")    { handleOper(fd, tokens);    return; }

    // Partner 2 — uncomment these once she has them ready:
    // if (cmd == "JOIN")   { handleJoin(fd, tokens);   return; }
    // if (cmd == "PART")   { handlePart(fd, tokens);   return; }
    // if (cmd == "KICK")   { handleKick(fd, tokens);   return; }
    // if (cmd == "INVITE") { handleInvite(fd, tokens); return; }
    // if (cmd == "TOPIC")  { handleTopic(fd, tokens);  return; }
    // if (cmd == "MODE")   { handleMode(fd, tokens);   return; }

    // Unknown command
    sendReply(fd, ":" SERVER_NAME " 421 " + _client[fd].getNick()
                  + " " + cmd + " :Unknown command");
}


// PASS <password>
// Must arrive before NICK/USER. Wrong password = instant disconnect.
void Server::handlePass(int fd, const std::vector<std::string>& tokens)
{
    auto& c = _client[fd];

    if (c.isRegistered()) {
        sendReply(fd, ":" SERVER_NAME " 462 " + c.getNick() + " :You may not reregister");
        return;
    }
    if (tokens.size() < 2) {
        sendReply(fd, ":" SERVER_NAME " 461 * PASS :Not enough parameters");
        return;
    }
    if (tokens[1] != _password) {
        sendReply(fd, ":" SERVER_NAME " 464 * :Password incorrect");
        disconnectClient(fd);
        return;
    }
    c.setPassOk(true);
}


bool Server::isNickValid(const std::string& nick) const
{
    if (nick.empty() || nick.size() > 9) return false;

    auto isSpecial = [](char c) {
        return std::string("[]\\^_`{|}").find(c) != std::string::npos;
    };

    if (!std::isalpha((unsigned char)nick[0]) && !isSpecial(nick[0]))
        return false;

    for (size_t i = 1; i < nick.size(); ++i) {
        char c = nick[i];
        if (!std::isalnum((unsigned char)c) && !isSpecial(c) && c != '-')
            return false;
    }
    return true;
}


// NICK <nickname>
// Works both before registration (setting nick for first time)
// and after (changing nick). Checks format + collision.
void Server::handleNick(int fd, const std::vector<std::string>& tokens)
{
    auto& c = _client[fd];

    if (tokens.size() < 2 || tokens[1].empty()) {
        sendReply(fd, ":" SERVER_NAME " 431 * :No nickname given");
        return;
    }

    const std::string& newNick = tokens[1];

    if (!isNickValid(newNick)) {
        sendReply(fd, ":" SERVER_NAME " 432 * " + newNick + " :Erroneous nickname");
        return;
    }

    // Is someone else already using this nick?
    auto it = _nickMap.find(newNick);
    if (it != _nickMap.end() && it->second != fd) {
        std::string who = c.getNick().empty() ? "*" : c.getNick();
        sendReply(fd, ":" SERVER_NAME " 433 " + who + " " + newNick + " :Nickname is already in use");
        return;
    }

    // Remove old nick from map, add new one
    if (!c.getNick().empty())
        _nickMap.erase(c.getNick());
    c.setNick(newNick);
    _nickMap[newNick] = fd;

    // If already registered, notify the client of the change
    if (c.isRegistered())
        sendReply(fd, getClientPrefix(fd) + " NICK " + newNick);
    else
        tryRegister(fd); // maybe this completes registration
}

// USER <username> <mode> <unused> :<realname>
// Example: USER alice 0 * :Alice Smith
// We only care about username (tokens[1]) and realname (tokens[4])
void Server::handleUser(int fd, const std::vector<std::string>& tokens)
{
    auto& c = _client[fd];

    if (c.isRegistered()) {
        sendReply(fd, ":" SERVER_NAME " 462 " + c.getNick() + " :You may not reregister");
        return;
    }
    if (tokens.size() < 5) {
        sendReply(fd, ":" SERVER_NAME " 461 * USER :Not enough parameters");
        return;
    }

    c.setUsername(tokens[1]);
    c.setRealname(tokens[4]);
    tryRegister(fd);
}


// Called after NICK and after USER.
// Registration is complete when: PASS ✓ + NICK set + USER set.
// Sends the 001 welcome message that IRC clients wait for.
void Server::tryRegister(int fd)
{
    auto& c = _client[fd];

    if (!c.hasPassOk() || c.getNick().empty() || c.getUsername().empty())
        return; // not ready yet

    if (c.isRegistered()) return; // already done

    c.setRegistered(true);

    sendReply(fd, ":" SERVER_NAME " 001 " + c.getNick()
                  + " :Welcome to the ft_irc Network "
                  + c.getNick() + "!" + c.getUsername() + "@" + c.getIp());
    sendReply(fd, ":" SERVER_NAME " 002 " + c.getNick()
                  + " :Your host is " SERVER_NAME ", running version 1.0");
}

// PRIVMSG <target> :<message>
// target is either a nickname ("bob") or a channel ("#general")
void Server::handlePrivmsg(int fd, const std::vector<std::string>& tokens)
{
    const std::string& senderNick = _client[fd].getNick();

    if (tokens.size() < 2) {
        sendReply(fd, ":" SERVER_NAME " 411 " + senderNick + " :No recipient given");
        return;
    }
    if (tokens.size() < 3 || tokens[2].empty()) {
        sendReply(fd, ":" SERVER_NAME " 412 " + senderNick + " :No text to send");
        return;
    }

    const std::string& target = tokens[1];
    const std::string& text   = tokens[2];

    // ── Channel message — Partner 2 handles this ───────────
    if (target[0] == '#') {
        // Uncomment once Partner 2 has Channel ready:
        // auto it = _channels.find(target);
        // if (it == _channels.end()) {
        //     sendReply(fd, ":" SERVER_NAME " 403 " + senderNick + " " + target + " :No such channel");
        //     return;
        // }
        // it->second->broadcast(getClientPrefix(fd) + " PRIVMSG " + target + " :" + text, fd);
        return;
    }

    // ── Direct message ─────────────────────────────────────
    auto it = _nickMap.find(target);
    if (it == _nickMap.end()) {
        sendReply(fd, ":" SERVER_NAME " 401 " + senderNick + " " + target + " :No such nick");
        return;
    }

    sendReply(it->second, getClientPrefix(fd) + " PRIVMSG " + target + " :" + text);
}


// OPER <username> <password>
// Elevates the client to server operator.
// Hardcode an oper password or store it however you like.
void Server::handleOper(int fd, const std::vector<std::string>& tokens)
{
    const std::string& nick = _client[fd].getNick();

    if (tokens.size() < 3) {
        sendReply(fd, ":" SERVER_NAME " 461 " + nick + " OPER :Not enough parameters");
        return;
    }

    // Change "operpass" to whatever password you want
    if (tokens[2] != "operpass") {
        sendReply(fd, ":" SERVER_NAME " 464 " + nick + " :Password incorrect");
        return;
    }

    _client[fd].setOper(true);
    sendReply(fd, ":" SERVER_NAME " 381 " + nick + " :You are now an IRC operator");
}
