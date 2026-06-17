#include <unistd.h>
#include <iostream>
#include <vector>
#include <sys/wait.h>

int main() {
    for(int i = 0; i<32; i++) {
        pid_t pid = fork();
        if(pid==-1) {
            std::cerr<<"fork failed\n";
        }
        else if(pid == 0) {
            execl("./client", "./client", "submit", "1", "1", "./sol.cpp", nullptr);
        } 
    } 
    while(waitpid(-1, NULL, 0)>0);
    return 0;
}
