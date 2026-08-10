#ifndef __SOCKET_HPP
#define __SOCKET_HPP


#include <sys/un.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include "config.hpp"

constexpr int MAXDATASIZE = 100;
constexpr int BACKLOG = 10;


#endif