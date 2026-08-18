#include "../include/isolate_utils.hpp"


// Isolate Init
Isolate_Init_status isolate_init() {
    Isolate_Init_status status;
    int curr_box = box_cnt.fetch_add(1) % 1000;
    status.box_id = std::to_string(curr_box);
    status.box_path = "/var/local/lib/isolate/" + status.box_id + "/box/";
    std::string box_id_init_arg = (std::string)"--box-id=" + status.box_id;
    
    //Initialize the box
    std::vector<char*> init_args = {
        (char*)"isolate",
        (char*)"--init",
        (char*)"--cg",
        const_cast<char*>(box_id_init_arg.c_str()),
        NULL
    }; 
    status.status = new_process("/usr/local/bin/isolate", init_args.data(), -1, STDOUT_FILENO, STDERR_FILENO);
    return status;
}


//Isolate run
int isolate_run(
    const std::string& box_id,
    const std::string& binary_file,
    const std::string& input_file
) { 
    const auto box_id_init_arg = (std::string)"--box-id=" + box_id;
    std::string metadata_file_path = temp_dir + box_id + (std::string)".meta";
    std::string metadata_init = (std::string)"--meta=" + metadata_file_path;
    std::string stdin_arg = (std::string)"--stdin=" + input_file;
    std::string stdout_arg = (std::string)"--stdout=out" + box_id + (std::string)".txt";
    std::vector<char*> run_args = {
        (char*)"isolate",
        const_cast<char*>(box_id_init_arg.c_str()),
        (char*)"--run", 
        (char*)"--cg",
        const_cast<char*>(metadata_init.c_str()),
        const_cast<char*>(stdin_arg.c_str()),
        const_cast<char*>(stdout_arg.c_str()), 
    };
    for(const auto arg:isolate_run_args) {
        run_args.emplace_back(arg);
    }
    run_args.push_back((char*)"--");
    run_args.push_back(const_cast<char*>(binary_file.c_str()));
    run_args.push_back(NULL);
    return new_process(
        "/usr/local/bin/isolate", 
        run_args.data(),
        -1, STDOUT_FILENO, STDERR_FILENO
    ); 
}



//Isolate cleanup
int isolate_cleanup(const std::string& boxid) {
    std::string box_init = "--box-id=" + boxid;
    std::vector<char*> args = {
        (char*)"isolate",
        (char*)"--cleanup",
        (char*)"--cg",
        const_cast<char*>(box_init.c_str()),
        NULL
    };
    return new_process(
        "/usr/local/bin/isolate",
        args.data(), -1, STDOUT_FILENO, STDERR_FILENO
    );
}



//metadata verdict
int metadata_verdict(const std::string& metadata_file_path) {
    exitsig.clear();
    std::ifstream file(metadata_file_path); 
    if(!file.is_open()) {
        return PROCESS_ERROR;
    }
    std::string key, value, status;
    int exitcode = 0;
    while (std::getline(file, key, ':') && std::getline(file, value)) {
        if(key == "cg-oom-killed") return MLE;
        else if(key == "status") status = value;
        else if(key == "exitsig") exitsig = value;
        else if(key == "exitcode") exitcode = std::stoi(value);
    }
    if(exitcode) {
        exitsig = std::to_string(-exitcode);
        return RUNTIME_ERROR;
    }
    if(!exitsig.empty()) return RUNTIME_ERROR;
    if(status == "TO") return TLE; 
    return 0;
}