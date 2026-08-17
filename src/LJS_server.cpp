#include <iostream>
#include <signal.h>
#include <vector>
#include "../include/socket.hpp"
#include "../include/threadpool.hpp"
#include "../include/judge.hpp"

void new_connection(int cfd, const std::string& client_cwd, uid_t client_uid) {
    if(send_client(cfd, "\033[36mJudging...\033[0m\n") == -1) return;
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

    if(argv.size() != 4) {
        send_client(cfd, "Incorrect Number of Arguments\n");
        std::cout<<"Connection Ended\n";
        close(cfd);
        return;
    }

    std::string cmd = argv[0];
 
    if(cmd == "run") {
        run(cfd, argv, client_cwd, client_uid);
    }
    else if(cmd == "submit") {
        submit(cfd, argv, client_cwd, client_uid);
    }
    else {
        std::cout << "options are:\n";
        std::cout << "    run\n";
        std::cout << "    submit\n";
    }
    std::cout<<"Connection Ended\n";
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

    threadpool pool(max_threads);

    std::cout<<"waiting for connections...\n";

    while(true) {
        int cfd;
        if((cfd = accept(sfd, nullptr, 0)) == -1) {
            perror("server: accept");
            continue;
        }

        struct ucred cred;
        socklen_t len = sizeof(cred);

        if (getsockopt(
            cfd,
            SOL_SOCKET,
            SO_PEERCRED,
            &cred,
            &len
        ) == -1) {

            perror("getsockopt");
            close(cfd);
            continue;
        }

        pid_t client_pid = cred.pid;
        uid_t client_uid = cred.uid;

        std::cout<<"Connection Established with pid " <<client_pid<<'\n';

        char cwd[MAX_PATH];

        std::string proc_cwd =
            "/proc/" +
            std::to_string(client_pid) +
            "/cwd";

        ssize_t pathlen = readlink(
            proc_cwd.c_str(),
            cwd,
            sizeof(cwd) - 1
        );

        if(pathlen == -1) {
            perror("readlink");
            close(cfd);
            continue;
        }

        cwd[pathlen] = '\0';

        std::string client_cwd(cwd);

        send_client(cfd, "\033[36mIn queue...\033[0m\n");

        pool.submit([=](){new_connection(cfd, client_cwd, client_uid);});
    } 

    return 0;
}
