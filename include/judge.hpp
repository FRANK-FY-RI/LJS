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
        send_client(cfd, "Unble to run some program\n\n");
    }
    else if(status == TLE) {
        send_client(cfd, "Time Limit Exceeded\n\n");
    }
    else if(status == MLE) {
        send_client(cfd, "Memory Limit Exceeded\n\n");
    }
    else if(status == RUNTIME_ERROR) {
        int exitcode = std::stoi(exitsig); 
        std::string msg;
        if(exitcode<0) {
            msg = static_cast<std::string>("Program exited with exit code ")
            + std::to_string(-exitcode) + static_cast<std::string>("\n\n");     
        }
        else msg = strsignal(std::stoi(exitsig)) + static_cast<std::string>("\n\n");
        send_client(cfd, msg);
    }
    else if(status == PROCESS_ERROR) {
        send_client(cfd, "Process Error\n\n");
    }
    else if(status == WA) {
        send_client(cfd, "Wrong answer\n\n");
    }
    else send_client(cfd, "Passed\n\n");
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
    std::string metadata_file = boxid + (std::string)".meta";
    std::string metadata_file_path = temp_dir + metadata_file;
    status = metadata_verdict(metadata_file_path);
    rm(metadata_file_path);
    if(status == CHILD_PROCESS_ERROR) {
        std::string msg = static_cast<std::string>("Error opening metadata file ") + (std::string)"\n";
        if(send_client(cfd, msg)==-1) return PROCESS_ERROR;
        return CHILD_PROCESS_ERROR;
    }
    if(status) return status;

    const std::string output_file = (std::string)"out" + boxid + ".txt";
    const std::string output_file_path = temp_dir + output_file;
 

    //check the output and answer  
    status = diff(answer_file_path, output_file_path); 
    rm(output_file_path); 
    if(status == PROCESS_ERROR) {
        send_client(cfd, "Unable to open output or answer files\n");
        return PROCESS_ERROR;
    } 
    else if(status == 0) return AC;
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
    std::string binary_path = temp_dir + binary;
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
         
        std::string msg = static_cast<std::string>("Test ") + 
        std::to_string(i) + static_cast<std::string>(": ");
        if(send_client(cfd, msg)==-1) return PROCESS_ERROR;
        if(!status) ac++;
        error_msg(cfd, status); 
        i++; 
    } 
    return 0;
}


//run command
int run(int cfd, std::vector<std::string> &argv, const std::string& client_cwd) { 
    std::string lab = (std::string)"Lab" + argv[1];
    std::string prob = (std::string)"prob_" + argv[2]; 
    std::string tc_path = prob_dir + lab + (std::string)"/Problem/" + prob + (std::string)"/"; 
    std::string code_path = client_cwd + (std::string)"/" + (std::string)argv[3]; 
    return runfn(cfd, tc_path, code_path);
}


//submit
int submit(int cfd, std::vector<std::string> &argv, const std::string& client_cwd) { 
    std::string lab = (std::string)"Lab" + argv[1];
    std::string prob = (std::string)"prob_" + argv[2]; 
    std::string tc_ex_path = prob_dir + lab + (std::string)"/Problem/" + prob + (std::string)"/";
    std::string tc_path = prob_dir + lab + (std::string)"/Hidden/" + prob + (std::string)"/"; 
    std::string code_path = client_cwd + (std::string)"/" + argv[3];
    
    //first check if ex_tc passes
    if(runfn(cfd, tc_ex_path, code_path) != AC) {
        if(send_client(cfd, "Example Test Case Failed\n")==-1) return PROCESS_ERROR;
        return WA;
    } 
    return runfn(cfd, tc_path, code_path);
}


#endif