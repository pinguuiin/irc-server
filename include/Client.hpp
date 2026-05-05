
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Server.hpp"
#include "Channel.hpp"
#include <string>
#include <string_view>
#include <map>

struct Modes
{
	bool away;
	bool invisible;
	bool wallops;
	bool restricted;
	bool op;
	bool localOp;
	bool server;
};

class Client
{
private:
    int _clientFd;
    bool _isRegistered;
    bool _isConnected;
    bool _isOperator;
    std::string _nickName;
    std::string _userName;
    std::string _fullName;
    const std::string _host;
    Modes _modes;
    std::map<std::string, Channel*> _joinedChannels;
    std::map<std::string, Channel*> _invitedChannels;
    std::string _sendBuffer;
    std::string _recvBuffer;

public:
    Client() noexcept;
    Client(int fd, std::string host);
    Client(const Client& other);
    Client(Client&& other) noexcept;
    ~Client() noexcept;
    Client& operator=(const Client& other);
    Client& operator=(Client&& other) noexcept;

    // Getters
    int getClientFd() const noexcept;
    bool getRegistration() const noexcept;
    bool getConnection() const noexcept;
    const std::string& getNickName() const noexcept;
    const std::string& getUserName() const noexcept;
    const std::string& getFullName() const noexcept;
    const std::string& getHost() const noexcept;
    std::string getUserPrefix() const;
    const Modes& getModes() const noexcept;
    const std::map<std::string, Channel*>& getJoinedChannels() const noexcept;
    const std::map<std::string, Channel*>& getInvitedChannels() const noexcept;

    // Setters
    void setClientFd(int fd) noexcept;
    void setRegistration(bool value = true) noexcept;
    void setConnection(bool value = true) noexcept;
    void setNickName(std::string nickName);
    void setUserName(std::string userName);
    void setFullName(std::string fullName);
    void setOperator(bool value = true) noexcept;
    std::string setMode(std::string mode);
    void addInvitedChannel(std::string channelName, Channel* channel);
    void addChannel(std::string channelName, Channel* channel);
    void partFromChannel(Channel* channel);
    bool isInvitedTo(Channel* invitedChannel) const;
    bool hasNoChannel() const noexcept;


};

#endif
