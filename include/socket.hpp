#ifndef __SOCKET_HPP
#define __SOCKET_HPP


#include <sys/un.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include "config.hpp"

constexpr int MAXDATASIZE = 100;
constexpr int BACKLOG = 10;


inline int send_client(int cfd, const std::string& msg) {
    if(send(cfd, msg.c_str(), msg.size(), 0) == -1) {
        perror("server: send");
        return -1;
    }
    return 0;
}


#endif