#include "socket.hpp"
#include <unistd.h>
#include <iostream>

bool isAlphaNumeric(const std::string& s) noexcept {
    for (char c : s) {
        if (!isalnum(c)) return false;
    }
    return true;
}

int main(int argc, char *argv[]) {

    if(argc == 1 || strcmp(argv[1], "--help")==0) {
        std::cout << "LJS — Lab Judge System\n";
        std::cout << "                     -By Omkar Singh\n\n";
        std::cout << "Usage:\n";
        std::cout << "  LJS <option> <Lab number> <problem number> <source code>\n";
        std::cout << "   options are:\n";
        std::cout << "       run\n";
        std::cout << "       custom_run\n";
        std::cout << "       submit\n";
        std::cout << "Examples:\n";
        std::cout << "  LJS run 1 1 prob_1.cpp\n";
        return 0;
    } 

    if(strcmp(argv[1], "custom_run")==0) {
        if(argc != 3) {
            std::cout<<"Usage:\n";
            std::cout<<"LJS custom_run <source code>\n";
            return 1;
        }

    }
    else if(strcmp(argv[1], "run")==0) {
        if(argc != 5) {
            std::cout<<"Usage:\n";
            std::cout<<"LJS run <Lab number> <Problem number> <source code>\n";
            return 1; 
        }
    }
    else if(strcmp(argv[1], "submit")) {
        if(argc != 5) {
            std::cout<<"Usage:\n";
            std::cout<<"LJS submit <Lab number> <Problem number> <source code>\n";
            return 1;
        }
    }
    else {
        std::cout<<"Unknown command\n";
        std::cout << "Usage:\n";
        std::cout << "  LJS <option> <Lab number> <problem number> <source code>\n";
        std::cout << "   options are:\n";
        std::cout << "       run\n";
        std::cout << "       custom_run\n";
        std::cout << "       submit\n";
        std::cout << "Examples:\n";
        std::cout << "  LJS run 1 1 prob_1.cpp\n";
    }

    int sfd;
    if((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_ADDR, sizeof(addr.sun_path)-1);

    if(connect(
        sfd, reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr.sun_path))==-1)
    {
        perror("connect");
        return 1;
    }

    if(strcmp(argv[1], "run")==0 || strcmp(argv[1], "submit")==0) {
        if(!isAlphaNumeric(argv[2]) || !isAlphaNumeric(argv[3])) {
            std::cerr<<"Lab Number and Problem Number must be AlphaNumeric\n";
            return 1;
        } 
    } 

    //send the arguments to the judge
    for(int i = 1; i<argc; i++) {
        int bytes_sent = send(sfd, argv[i], strlen(argv[i]), 0);
        if(bytes_sent == -1) {
            perror("send");
            return 1;
        }
        if(bytes_sent != strlen(argv[i])) {
            perror("partial send");
            return 1;
        }
    }

    //print the verdict from the judge
    char buf[MAXDATASIZE];
    int bytes_read;
    while((bytes_read = recv(sfd, buf, MAXDATASIZE, 0)) > 0) {
        if(write(STDOUT_FILENO, buf, bytes_read) != bytes_read) {
            perror("partial/failed write"); 
        }
    }
    if(bytes_read == -1) {
        perror("recv");
    }
    close(sfd);
    return 0;
}