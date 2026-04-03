#include "process_utils.hpp"


using std::pair;
using std::string;



//fucntion to start a new process
int new_process(const char* path, char *args[], const int input_fd, const int output_fd) {
    int status;
    pid_t pid = fork();
    int ret_status;
    if(pid == 0) {
        //redirect input
        if(input_fd != -1) {
            dup2(input_fd, 0);
            close(input_fd);
        }

        //redirect output
        if(output_fd != -1) {
            dup2(output_fd, 1);
            close(output_fd);
        }

        execv(path, args);
        exit(CHILD_PROCESS_ERROR);
    }
    else {
        waitpid(pid, &status, 0);
        if(WIFEXITED(status)) {
            ret_status = WEXITSTATUS(status);
        } 
        else ret_status = CHILD_PROCESS_ERROR;
    }
    return ret_status;
}



//Compile function
pair<int, string> compile(const char *code) {
    const string binary = "sol";
    char *compile_args[] = {
        (char*)"g++",
        (char*)"-std=c++23",
        (char*)"-Wall",
        const_cast<char*>(code),
        (char*)("-o"),
        const_cast<char*>(binary.c_str()),
        (char*)"-static-libstdc++",
        NULL
    };
    int status = new_process("/usr/bin/g++", compile_args, -1, -1); 
    return {status, binary};
}


