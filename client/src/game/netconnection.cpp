#include "common.h"
#include "logger.h"
#include "netconnection.h"
#include "snprintf.h"

std::vector<const char*> NetConnection::strtab;
bool NetConnection::initialised = false;
Uint8 NetConnection::messages = 5;

void NetConnection::initMessages() {
#ifndef NONET
    if(SDLNet_Init()==-1) {
        logger->error("SDLNet_Init: %s\n", SDLNet_GetError());
        throw 0;
    }
    strtab.push_back("version:");
    strtab.push_back("nick:");
    strtab.push_back("side:");
    strtab.push_back("colour:");
    strtab.push_back("terminates\n");
    initialised = true;
#endif
}

void NetConnection::quit()
{
#ifndef NONET
    SDLNet_Quit();
#endif
}

NetConnection::NetConnection(const char *address, Uint16 port)
{
    loggedin = false;
#ifndef NONET
    SDLNet_ResolveHost(&ip, const_cast<char*>(address), port);
    tcpsock = SDLNet_TCP_Open(&ip);
    if (!tcpsock) {
        logger->error("Can't connect. Reason: %s\n", SDLNet_GetError());
        throw 0;
    }
#else
    logger->error("This version doesn't support networking, plase recompile\n");
    throw 0;
#endif

}

NetConnection::~NetConnection()
{
#ifndef NONET
    if (loggedin) {
        logout();
    }
    SDLNet_TCP_Close(tcpsock);
#endif
}

/** @brief Sends player info to server for confirmation.
 * @returns true if logged in successfully.
 */
bool NetConnection::login(const char* version, const char* nick, const char* side, const char* colour)
{
#ifndef NONET
    Uint8 result;
    int len;
    char* tmp;
    const char* args[4] = {version, nick, side, colour};
    logger->note("Logging into server: ");
    for (Uint8 a=0; a < 4; ++a) {
        len = asprintf(&tmp, "%s %s\n",strtab[a],args[a]);
        if (len == -1) {
            return false;
        }
        result = send((const Uint8*)tmp, len);
        if (result < (unsigned)len) {
            logger->error("Socket: %s\n", SDLNet_GetError());
            return false;
        }
        free(tmp);
    }
    loggedin = true;
    return true;
#else
    return false;
#endif
}

void NetConnection::logout()
{
#ifndef NONET
    send((const Uint8*)(strtab[logout_str]), strlen(strtab[logout_str]));
    loggedin = false;
#endif
}

Uint16 NetConnection::send(const Uint8 *data, Uint16 len)
{
#ifndef NONET
    return SDLNet_TCP_Send(tcpsock, const_cast<Uint8*>(data), len);
#else
    return 0;
#endif
}

Uint16 NetConnection::recv(Uint8 *data, Uint16 len)
{
#ifndef NONET
    return SDLNet_TCP_Recv(tcpsock, data, len);
#else
    return 0;
#endif
}

