#ifndef __JUDGE_HPP
#define __JUDGE_HPP


#include "process_utils.hpp"
#include "isolate_utils.hpp"
#include "socket.hpp"
#include <iostream>
#include <vector>


inline int send_client(int cfd, const std::string& msg) {
    if(send(cfd, msg.c_str(), msg.size(), 0) == -1) {
        perror("server: send");
        return -1;
    }
    return 0;
}


//error message
void error_msg(int cfd, int status) {
    if(status == CHILD_PROCESS_ERROR) {
        if(send_client(cfd, "Unble to run some program\n")==-1) return;
    }
    else if(status == TLE) {
        if(send_client(cfd, "Time Limit Exceeded\n")==-1) return;
    }
    else if(status == MLE) {
        if(send_client(cfd, "Memory Limit Exceeded\n")==-1) return;
    }
    else if(status == RUNTIME_ERROR) {
        std::string msg = strsignal(std::stoi(exitsig)) + static_cast<std::string>("\n");
        if(send_client(cfd, msg)==-1) return;
    }
    else if(status == PROCESS_ERROR) {
        if(send_client(cfd, "Process Error\n")==-1) return;
    }
}

//judge function
int judge(int cfd, const std::string& binary, const std::string& binary_path, const std::string& input_file, const std::string& input_file_path, const std::string& answer_file_path) { 
 
    //run
    auto [status, boxid] = isolate_run(cfd, binary, binary_path, input_file, input_file_path);
    if(status == CHILD_PROCESS_ERROR) {
        if(send_client(cfd, "Unable to spawn new process: isolate sandbox\n")==-1) return PROCESS_ERROR;
        return status;
    } 
    if(status == PROCESS_ERROR) {
        if(send_client(cfd, "Error running process: isolate sandbox\n")==-1) return PROCESS_ERROR;
        return status;
    } 

    //check the metadata verdict
    std::string metadata_file = (std::string)"metadata" + boxid + (std::string)".meta";
    std::string metadata_file_path = (std::string)"/tmp/" + metadata_file;
    status = metadata_verdict(metadata_file_path);
    rm(metadata_file_path);
    if(status == CHILD_PROCESS_ERROR) {
        std::string msg = static_cast<std::string>("Error opening file") + metadata_file_path;
        if(send_client(cfd, msg)==-1) return PROCESS_ERROR;
        return CHILD_PROCESS_ERROR;
    }
    error_msg(cfd, status);
    if(status) return status;

    const std::string output_file = (std::string)"out" + boxid + ".txt";
    const std::string output_file_path = (std::string)"/tmp/" + output_file;
 

    //check the output and answer  
    status = diff(answer_file_path, output_file_path); 
    rm(output_file_path); 
    if(!status) {
        return AC;
    }
    return WA; 
}



//run function
int runfn(int cfd, const std::string& tc_path, const std::string& code) {
    
    //compile
    auto [compile_status, binary] = compile(cfd, code.c_str());
    if(compile_status == CHILD_PROCESS_ERROR) {
        if(send_client(cfd, "Unable to spawn new process: g++\n")==-1) return PROCESS_ERROR;
        return CHILD_PROCESS_ERROR;
    }
    if(compile_status == PROCESS_ERROR) {
        if(send_client(cfd, "Compilation Error\n")==-1) return PROCESS_ERROR;
        return compile_status;
    }
    std::string binary_path = (std::string)"./" + binary;
    if(compile_status) {
        send_client(cfd, "Compilation error\n");
        rm(binary_path);
        return compile_status;
    }          
    
    int i = 1;
    int ac = 0;
    while(true) {
        std::string input_file = std::to_string(i) + ".in";
        std::string input_file_path = tc_path + input_file;
        std::string answer_file = std::to_string(i) + ".ans";
        std::string answer_file_path = tc_path + answer_file;
        
        //Check if file exists
        bool file_exists = false;
        if(access(input_file_path.c_str(), F_OK) == 0 ) file_exists = true;
        if(!file_exists) {
            if(ac == i-1) {
                if(send_client(cfd, "✅ ")==-1) return PROCESS_ERROR;
            }
            else {
                if(send_client(cfd, "❌ ")==-1) return PROCESS_ERROR;
            }
            std::string msg = std::to_string(ac) + static_cast<std::string>("/") + 
            std::to_string(i-1) + static_cast<std::string>("Passed\n");
            if(send_client(cfd, msg)==-1) return PROCESS_ERROR;
            rm(binary_path);
            if(ac == (i-1)) return AC;
            return WA;
        }
        if(access(answer_file_path.c_str(), F_OK) != 0) {
            std::string msg = static_cast<std::string>("Answer file ") + 
            std::to_string(i) + static_cast<std::string>(" not present\n");
            if(send_client(cfd, msg)==-1) return PROCESS_ERROR;
            i++;
            continue;
        }
        
        int status = judge(cfd, binary, binary_path, input_file, input_file_path, answer_file_path);
        
        if(status == CHILD_PROCESS_ERROR) {
            rm(binary_path);
            return CHILD_PROCESS_ERROR;
        }
        std::string msg = static_cast<std::string>("Test ") + 
        std::to_string(i) + static_cast<std::string>(": ");
        if(send_client(cfd, msg)==-1) return PROCESS_ERROR;
        if(!status) {
            if(send_client(cfd, "Passed\n")==-1) return PROCESS_ERROR;
            ac++;
        }
        else {
            if(send_client(cfd, "Wrong Answer\n")==-1) return PROCESS_ERROR;
        }
        i++; 
    } 
    return 0;
}


//run command
int run(int cfd, std::vector<std::string> &argv) { 
    std::string lab = (std::string)"Lab" + argv[1];
    std::string prob = (std::string)"prob_" + argv[2]; 
    
    
    std::string tc_path = (std::string)"./" + lab + (std::string)"/Problem/" + prob + (std::string)"/"; 
    
    return runfn(cfd, tc_path, argv[3]);
}


//submit
int submit(int cfd, std::vector<std::string> &argv) { 
    std::string lab = (std::string)"Lab" + argv[1];
    std::string prob = (std::string)"prob_" + argv[2]; 
    std::string tc_ex_path = (std::string)"./" + lab + (std::string)"/Problem/" + prob + (std::string)"/";
    std::string tc_path = (std::string)"./" + lab + (std::string)"/Hidden/" + prob + (std::string)"/"; 
    
    //first check if ex_tc passes
    if(runfn(cfd, tc_ex_path, argv[3]) == WA) {
        if(send_client(cfd, "Example Test Case Failed\n")==-1) return PROCESS_ERROR;
        return WA;
    } 
    return runfn(cfd, tc_path, argv[3]);
}


#endif