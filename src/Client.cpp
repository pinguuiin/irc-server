#include "../include/Client.hpp"
#include <utility> // for std::move
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <iostream>

Client::Client() noexcept
    : _clientFd(-1), _isRegistered(false), _isConnected(false), _isOperator(false), _host("") {}

Client::Client(int fd, std::string host)
    : _clientFd(fd), _isRegistered(false), _isConnected(true), _isOperator(false), _host(std::move(host)) {}

Client::Client(const Client& other)
    : _clientFd(other._clientFd), _isRegistered(other._isRegistered), _isConnected(other._isConnected),
      _isOperator(other._isOperator), _nickName(other._nickName), _userName(other._userName),
      _fullName(other._fullName), _host(other._host), _modes(other._modes),
      _joinedChannels(other._joinedChannels), _invitedChannels(other._invitedChannels),
      _sendBuffer(other._sendBuffer), _recvBuffer(other._recvBuffer) {}

Client::Client(Client&& other) noexcept
    : _clientFd(other._clientFd), _isRegistered(other._isRegistered), _isConnected(other._isConnected),
      _isOperator(other._isOperator), _nickName(std::move(other._nickName)), _userName(std::move(other._userName)),
      _fullName(std::move(other._fullName)), _host(std::move(other._host)), _modes(other._modes),
      _joinedChannels(std::move(other._joinedChannels)), _invitedChannels(std::move(other._invitedChannels)),
      _sendBuffer(std::move(other._sendBuffer)), _recvBuffer(std::move(other._recvBuffer)) {
    other._clientFd = -1;
    other._isRegistered = false;
    other._isConnected = false;
    other._isOperator = false;
}

Client::~Client() noexcept {}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        _clientFd = other._clientFd;
        _isRegistered = other._isRegistered;
        _isConnected = other._isConnected;
        _isOperator = other._isOperator;
        _nickName = other._nickName;
        _userName = other._userName;
        _fullName = other._fullName;
        // _host is const, can't assign
        _modes = other._modes;
        _joinedChannels = other._joinedChannels;
        _invitedChannels = other._invitedChannels;
        _sendBuffer = other._sendBuffer;
        _recvBuffer = other._recvBuffer;
    }
    return *this;
}

Client& Client::operator=(Client&& other) noexcept {
    if (this != &other) {
        _clientFd = other._clientFd;
        _isRegistered = other._isRegistered;
        _isConnected = other._isConnected;
        _isOperator = other._isOperator;
        _nickName = std::move(other._nickName);
        _userName = std::move(other._userName);
        _fullName = std::move(other._fullName);
        // _host const
        _modes = other._modes;
        _joinedChannels = std::move(other._joinedChannels);
        _invitedChannels = std::move(other._invitedChannels);
        _sendBuffer = std::move(other._sendBuffer);
        _recvBuffer = std::move(other._recvBuffer);
        other._clientFd = -1;
        other._isRegistered = false;
        other._isConnected = false;
        other._isOperator = false;
    }
    return *this;
}

// Getters
int Client::getClientFd() const noexcept { return _clientFd; }
bool Client::getRegistration() const noexcept { return _isRegistered; }
bool Client::getConnection() const noexcept { return _isConnected; }
const std::string& Client::getNickName() const noexcept { return _nickName; }
const std::string& Client::getUserName() const noexcept { return _userName; }
const std::string& Client::getFullName() const noexcept { return _fullName; }
const std::string& Client::getHost() const noexcept { return _host; }
std::string Client::getUserPrefix() const { return _nickName + "!" + _userName + "@" + _host; }
const Modes& Client::getModes() const noexcept { return _modes; }
const std::map<std::string, Channel*>& Client::getJoinedChannels() const noexcept { return _joinedChannels; }
const std::map<std::string, Channel*>& Client::getInvitedChannels() const noexcept { return _invitedChannels; }

// Setters
void Client::setClientFd(int fd) noexcept { _clientFd = fd; }
void Client::setRegistration(bool value) noexcept { _isRegistered = value; }
void Client::setConnection(bool value) noexcept { _isConnected = value; }
void Client::setNickName(std::string nickName) { _nickName = std::move(nickName); }
void Client::setUserName(std::string userName) { _userName = std::move(userName); }
void Client::setFullName(std::string fullName) { _fullName = std::move(fullName); }
void Client::setOperator(bool value) noexcept { _isOperator = value; }

std::string Client::setMode(std::string mode) {
    // Simple implementation, assume mode is like "+i" or "-i"
    if (mode.size() >= 2) {
        char sign = mode[0];
        char flag = mode[1];
        bool set = (sign == '+');
        switch (flag) {
            case 'a': _modes.away = set; break;
            case 'i': _modes.invisible = set; break;
            case 'w': _modes.wallops = set; break;
            case 'r': _modes.restricted = set; break;
            case 'o': _modes.op = set; break;
            case 'O': _modes.localOp = set; break;
            case 's': _modes.server = set; break;
        }
    }
    return mode; // Return the mode string, perhaps for response
}

void Client::addInvitedChannel(std::string channelName, Channel* channel) {
    _invitedChannels[std::move(channelName)] = channel;
}

void Client::addChannel(std::string channelName, Channel* channel) {
    _joinedChannels[std::move(channelName)] = channel;
}

void Client::partFromChannel(Channel* channel) {
    // Assuming Channel has getName()
    auto it = _joinedChannels.find(channel->getName());
    if (it != _joinedChannels.end()) {
        _joinedChannels.erase(it);
    }
}

bool Client::isInvitedTo(Channel* invitedChannel) const {
    // Assuming Channel has getName()
    return _invitedChannels.find(invitedChannel->getName()) != _invitedChannels.end();
}

bool Client::hasNoChannel() const noexcept {
    return _joinedChannels.empty();
}

