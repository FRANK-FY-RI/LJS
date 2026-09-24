#include <iostream>
#include <signal.h>
#include <vector>
#include "../include/socket.hpp"
#include "../include/threadpool.hpp"
#include "../include/judge.hpp"
#include "../include/verdict.hpp"


void new_connection(ClientContext client, const LJSdatabase& database) {
    if(send_client(client.cfd, "\033[36mJudging...\033[0m\n") == Verdict::FAILURE) return;
    char buf[MAXDATASIZE+1];
    int bytes_read;
    std::string msg;
    while((bytes_read = recv(client.cfd, buf, MAXDATASIZE, 0)) > 0) {
        buf[bytes_read] = '\0';
        msg += buf;
    }
    if(bytes_read == -1) {
        perror("recv");
        close(client.cfd);
        return;
    }

    std::string temp;
    std::vector<std::string> argv;
    bool got_cwd = true;
    for(auto it:msg) {
        if(it == '\n') {
            if(got_cwd) {
                client.cwd = temp;
                got_cwd = false;
            }
            else argv.emplace_back(temp);
            temp.clear();
        }
        else temp += it;
    }

    if(argv.size() != 4) {
        send_client(client.cfd, "Incorrect Number of Arguments\n");
        std::cout<<"Connection Ended\n";
        close(client.cfd);
        return;
    }

    std::string cmd = argv[0];
 
    if(cmd == "run") {
        run(client, argv);
    }
    else if(cmd == "submit") {
        submit(client, argv, database);
    }
    else {
        std::cout << "options are:\n";
        std::cout << "    run\n";
        std::cout << "    submit\n";
    }
    std::cout<<"Connection Ended\n";
    close(client.cfd);
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

    Database db(database_dir + "LJS.db");
    if(db.schema_init(database_dir + "db_schema.sql")) {
        std::cerr<<"Unable to initialize the schema\n";
        return 1;
    }
    const LJSdatabase database(db, "submissions", "codes");

    ClientContext client;

    while(true) {
        int cfd;
        if((cfd = accept(sfd, nullptr, 0)) == -1) {
            perror("server: accept");
            continue;
        }

        client.cfd = cfd;

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

        client.uid = cred.uid;

        std::cout<<"Connection Established with pid " <<client_pid<<'\n';

        send_client(cfd, "\033[36mIn queue...\033[0m\n");

        pool.submit([client, &database](){new_connection(client, database);});
    } 

    return 0;
}
