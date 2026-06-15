#ifndef __SOCKET_HPP
#define __SOCKET_HPP


#include <sys/un.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

constexpr char SV_SOCK_ADDR[] = "/tmp/sv_sock_addr";
constexpr int MAXDATASIZE = 100;
constexpr int BACKLOG = 10;


#endif