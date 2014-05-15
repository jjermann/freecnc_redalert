// mode: -*- C++ -*-
#ifndef MESSAGE_H
#define MESSAGE_H

#include <deque>
#include "SDL.h"
#include "common.h"

class Font;

class Message
{
public:
    Message(const char *msg, Uint32 deltime)
    {
        message = cppstrdup(msg);
        this->deltime = deltime;
    }
    ~Message()
    {
        delete[] message;
    }
    const char *getMessage()
    {
        return message;
    }
    Uint32 getDeleteTime()
    {
        return deltime;
    }
private:
    char* message;
    Uint32 deltime;
};

class MessagePool
{
public:
    MessagePool();
    ~MessagePool();
    SDL_Surface *getMessages();
    void postMessage(const char *msg);
    static MessagePool *currentInstance;
    void clear();
    //Font *getFont(){return msgfont;}
private:
    std::deque<Message *> msglist;
    bool updated;
    SDL_Surface *textimg;
    Font *msgfont;
};

#endif
