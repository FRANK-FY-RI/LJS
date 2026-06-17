#include <iostream>
#include <signal.h>
#include <vector>
#include "../include/socket.hpp"
#include "../include/threadpool.hpp"
#include "../include/judge.hpp"

constexpr int n_threads = 11;

void new_connection(int cfd) {
    char buf[MAXDATASIZE+1];
    int bytes_read;
    std::string msg;
    while((bytes_read = recv(cfd, buf, MAXDATASIZE, 0)) > 0) {
        buf[bytes_read] = '\0';
        msg += buf;
    }
    if(bytes_read == -1) {
        perror("recv");
        close(cfd);
        return;
    }

    std::string temp;
    std::vector<std::string> argv;
    for(auto it:msg) {
        if(it == '\n') {
            argv.emplace_back(temp);
            temp.clear();
        }
        else temp += it;
    }

    std::string cmd = argv[0];
 
    if(cmd == "run") {
        run(cfd, argv);
    }
    else if(cmd == "submit") {
        submit(cfd, argv);
    }
    else {
        std::cout << "options are:\n";
        std::cout << "    run\n";
        std::cout << "    submit\n";
    }
    std::cout<<"connection ended\n";
    close(cfd);
}



int main() { 

    signal(SIGPIPE, SIG_IGN);

    int sfd;
    if((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("server: socket");
        return 1;
    }

    if(remove(SV_SOCK_ADDR) && errno!=ENOENT) {
        perror("remove");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_ADDR, sizeof(addr.sun_path)-1);

    if(bind(
        sfd, reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr.sun_path)
    )==-1)
    {
        perror("server: bind");
        return 1;
    }

    if(listen(sfd, BACKLOG) == -1) {
        perror("server: listen");
        return 1;
    }

    threadpool pool(n_threads);

    std::cout<<"waiting for connections...\n";

    while(true) {
        int cfd;
        if((cfd = accept(sfd, nullptr, 0)) == -1) {
            perror("server: accept");
            continue;
        }

        std::cout<<"connection established\n";

        pool.submit([cfd](){new_connection(cfd);});
    } 

    return 0;
}
