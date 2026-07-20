#include "../include/process_utils.hpp"




//function to start a new process
int new_process(const char* path, char *args[], const int input_fd, const int output_fd, const int error_fd) {
    int status;
    pid_t pid = fork();
    if(pid == -1) {
        return CHILD_PROCESS_ERROR; 
    }
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

        //redirect error
        if(error_fd != -1) {
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);
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
std::pair<int, std::string> compile(int cfd, const char *code) {
    const std::string binary = "sol" + std::to_string(cfd);
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
    int status = new_process("/usr/bin/g++", compile_args, -1, -1, cfd); 
    return {status, binary};
}


//Compare files
int diff(const std::string& file1_path, const std::string& file2_path) {
    std::ifstream file1(file1_path), file2(file2_path);
    std::string s1, s2, temp;
    auto rtrim = [](std::string& s) {
        int n = s.size();
        for(int i = n-1; i>=0; i--) {
            if(std::isgraph(s[i])) break;
            s.pop_back();
        }
    };
    while(std::getline(file1, temp)) {
        s1 += temp;
        s1 += '\n';
    } 
    while(std::getline(file2, temp)) {
        s2 += temp;
        s2 += '\n';
    } 
    rtrim(s1);
    rtrim(s2);
    return !(s1==s2);
}
