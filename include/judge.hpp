#ifndef __JUDGE_HPP
#define __JUDGE_HPP


#include "process_utils.hpp"
#include "socket.hpp"
#include "verdict.hpp"
#include <pwd.h>
#include <sys/stat.h>
#include <optional>
#include <string>
#include "../src/database/database.hpp"
#include "../src/database/table.hpp"



struct LJSdatabase {
    const Database& db;
    const Table submissions;
    const Table codes;
    LJSdatabase(const Database& database, const std::string& sub, const std::string& cod) : 
        db(database),
        submissions(db, sub),
        codes(db, cod)
    {}
};



//Hashing Function
std::string hash(const std::string&);


//error message
inline void error_msg(int cfd, Verdict status) {
    if(status == Verdict::CHILD_PROCESS_ERROR) {
        send_client(cfd, "\033[34mUnable to run some program\033[0m\n");
    }
    else if(status == Verdict::TLE) {
        send_client(cfd, "\033[91mTime Limit Exceeded\033[0m\n");
    }
    else if(status == Verdict::MLE) {
        send_client(cfd, "\033[91mMemory Limit Exceeded\033[0m\n");
    }
    else if(status == Verdict::RUNTIME_ERROR) {
        std::string msg;
        if(exitsig.empty()) {
            msg = "\033[91mProgram exited with unknown error\033[0m\n";
            send_client(cfd, msg);
            return;
        }
        int exitcode = std::stoi(exitsig); 
        if(exitcode<0) {
            msg = static_cast<std::string>("\033[91mProgram exited with exit code ")
            + std::to_string(-exitcode) + static_cast<std::string>("\033[0m\n");     
        }
        else msg = (std::string)"\033[91m" + strsignal(std::stoi(exitsig)) + static_cast<std::string>("\033[0m\n");
        send_client(cfd, msg);
    }
    else if(status == Verdict::PROCESS_ERROR) {
        send_client(cfd, "\033[34mProcess Error\033[0m\n");
    }
    else if(status == Verdict::WA) {
        send_client(cfd, "\033[31mWrong Answer\033[0m\n");
    }
    else if(status == Verdict::AC) send_client(cfd, "\033[32mAccepted\033[0m\n");
    else send_client(cfd, "\033[34mUnknown Error\033[0m\n");
}


// check source code from client's directory
std::optional<std::string> resolve_source(
    const std::string& cwd,
    const std::string& filename,
    uid_t uid
);

//judge function
Verdict judge(
    int cfd,
    const std::string& binary_file,
    const std::string& binary_file_path,
    const std::string& input_file,
    const std::string& input_file_path,
    const std::string& answer_file_path
); 



//run function
Verdict runfn(int cfd, const std::string& tc_path, const std::string& code); 


//run command
Verdict run(
    int cfd,
    std::vector<std::string> &argv,
    const std::string& client_cwd, uid_t client_uid
); 


//submit
Verdict submit(
    int cfd,
    std::vector<std::string> &argv,
    const std::string& client_cwd, uid_t client_uid,
    const LJSdatabase& database
);


#endif
