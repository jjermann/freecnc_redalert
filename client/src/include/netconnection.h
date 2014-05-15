#ifndef NETCONNECTION_H
#define NETCONNECTION_H

#include <vector>
#include "SDL_types.h"

#ifndef NONET
#include "SDL_net.h"
#endif

class NetConnection
{
public:
    NetConnection(const char *address, Uint16 port);
    ~NetConnection();
    bool login(const char* version, const char* nick, const char* side, const char* colour);
    void logout();
    Uint16 send(const Uint8 *data, Uint16 len);
    Uint16 recv(Uint8 *data, Uint16 len);
    static void initMessages();
    static void quit();
private:
    enum {login1,login2,login3,login4,logout_str};
    bool loggedin;
    static bool initialised;
    static std::vector<const char*> strtab;
    static Uint8 messages;
#ifndef NONET
    IPaddress ip;
    TCPsocket tcpsock;
#endif
};

#endif

