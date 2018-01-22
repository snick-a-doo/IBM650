#ifndef BUFFER_H
#define BUFFER_H

#include "register.hpp"

#include <deque>
#include <memory>

using Buffer = std::deque<IBM650::Word>;

class Source_Client;

class Source
{
public:
    virtual void connect_source_client(std::weak_ptr<Source_Client> client) = 0;
    virtual void advance() = 0;
    virtual Buffer& get_source() = 0;
};

class Source_Client
{
public:
    virtual void connect_source(std::weak_ptr<Source> source) = 0;
    virtual void resume() = 0;
};

#endif
