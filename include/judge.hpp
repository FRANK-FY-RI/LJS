#ifndef __JUDGE_HPP
#define __JUDGE_HPP


#include "process_utils.hpp"
#include "isolate_utils.hpp"
#include "socket.hpp"
#include <pwd.h>
#include <sys/stat.h>
#include <filesystem>
#include <optional>



//error message
inline void error_msg(int cfd, int status) {
    if(status == CHILD_PROCESS_ERROR) {
        send_client(cfd, "\033[34mUnable to run some program\033[0m\n");
    }
    else if(status == TLE) {
        send_client(cfd, "\033[91mTime Limit Exceeded\033[0m\n");
    }
    else if(status == MLE) {
        send_client(cfd, "\033[91mMemory Limit Exceeded\033[0m\n");
    }
    else if(status == RUNTIME_ERROR) {
        int exitcode = std::stoi(exitsig); 
        std::string msg;
        if(exitcode<0) {
            msg = static_cast<std::string>("\033[91mProgram exited with exit code ")
            + std::to_string(-exitcode) + static_cast<std::string>("\033[0m\n");     
        }
        else msg = (std::string)"\033[91m" + strsignal(std::stoi(exitsig)) + static_cast<std::string>("\033[0m\n");
        send_client(cfd, msg);
    }
    else if(status == PROCESS_ERROR) {
        send_client(cfd, "\033[34mProcess Error\033[0m\n");
    }
    else if(status == WA) {
        send_client(cfd, "\033[31mWrong Answer\033[0m\n");
    }
    else send_client(cfd, "\033[32mAccepted\033[0m\n");
}


// check source code from client's directory
std::optional<std::string> resolve_source(
    const std::string& cwd,
    const std::string& filename,
    uid_t uid
);

//judge function
int judge(
    int cfd,
    const std::string& binary_file,
    const std::string& binary_file_path,
    const std::string& input_file,
    const std::string& input_file_path,
    const std::string& answer_file_path
); 



//run function
int runfn(int cfd, const std::string& tc_path, const std::string& code); 


//run command
int run(
    int cfd,
    std::vector<std::string> &argv,
    const std::string& client_cwd, uid_t client_uid
); 


//submit
int submit(
    int cfd,
    std::vector<std::string> &argv,
    const std::string& client_cwd, uid_t client_uid
);


#endif
