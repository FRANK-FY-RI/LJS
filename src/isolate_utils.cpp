#include "../include/isolate_utils.hpp"
#include "../include/process_utils.hpp"
#include "../include/config.hpp"
#include <mutex>
#include <vector>


// Definitions of static data members
threadsafe_queue<int> Isolate::free_box_ids;
std::once_flag Isolate::free_box_ids_init_flag;


// Destructor
Isolate::~Isolate() {
    isolate_cleanup();
}

// Free box IDs queue Init
void Isolate::init_free_box_ids() {
    std::call_once(free_box_ids_init_flag, [](){
        for(int i = 0; i<1000; i++) {
            free_box_ids.push(i);
        }
    });
};

// Isolate Init
Verdict Isolate::isolate_init() {
    init_free_box_ids();
    Verdict status;
    int curr_box;
    free_box_ids.wait_and_pop(curr_box);
    box_id = std::to_string(curr_box);
    box_path = "/var/local/lib/isolate/" + box_id + "/box/";
    std::string box_id_init_arg = (std::string)"--box-id=" + box_id;
    
    //Initialize the box
    std::vector<char*> init_args = {
        (char*)"isolate",
        (char*)"--init",
        (char*)"--cg",
        const_cast<char*>(box_id_init_arg.c_str()),
        NULL
    }; 
    status = new_process("/usr/local/bin/isolate", init_args.data(), -1, STDOUT_FILENO, STDERR_FILENO);
    if(status == Verdict::SUCCESS) is_active = true;
    return status;
}


//Isolate run
Verdict Isolate::isolate_run(
    const std::string& binary_file,
    const std::string& input_file
) { 
    const auto box_id_init_arg = (std::string)"--box-id=" + box_id;
    std::string metadata_file_path = temp_dir + box_id + (std::string)".meta";
    std::string metadata_init = (std::string)"--meta=" + metadata_file_path;
    std::string stdin_arg = (std::string)"--stdin=" + input_file;
    std::string stdout_arg = (std::string)"--stdout=out" + box_id + (std::string)".txt";
    std::string stderr_arg = (std::string)"--stderr=err" + box_id + (std::string)".err";
    std::vector<char*> run_args = {
        (char*)"isolate",
        const_cast<char*>(box_id_init_arg.c_str()),
        (char*)"--run", 
        const_cast<char*>(metadata_init.c_str()),
        const_cast<char*>(stdin_arg.c_str()),
        const_cast<char*>(stdout_arg.c_str()),
        const_cast<char*>(stderr_arg.c_str()), 
        (char*)"--cg",
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
Verdict Isolate::isolate_cleanup() {
    std::string box_init = "--box-id=" + box_id;
    std::vector<char*> args = {
        (char*)"isolate",
        (char*)"--cleanup",
        (char*)"--cg",
        const_cast<char*>(box_init.c_str()),
        NULL
    };
    free_box_ids.push(std::stoi(box_id));
    return new_process(
        "/usr/local/bin/isolate",
        args.data(), -1, STDOUT_FILENO, STDERR_FILENO
    );
}



//metadata verdict
Verdict Isolate::metadata_verdict(const std::string& metadata_file_path) {
    exitsig.clear();
    std::ifstream file(metadata_file_path); 
    if(!file.is_open()) {
        return Verdict::PROCESS_ERROR;
    }
    std::string key, value, status;
    int exitcode = 0;
    while (std::getline(file, key, ':') && std::getline(file, value)) {
        if(key == "cg-oom-killed") return Verdict::MLE;
        else if(key == "status") status = value;
        else if(key == "exitsig") exitsig = value;
        else if(key == "exitcode") exitcode = std::stoi(value);
    }
    if(exitcode) {
        exitsig = std::to_string(-exitcode);
        return Verdict::RUNTIME_ERROR;
    }
    if(!exitsig.empty()) return Verdict::RUNTIME_ERROR;
    if(status == "TO") return Verdict::TLE; 
    return Verdict::SUCCESS;
}



//Isolate Compilation
Compile_Status Isolate::isolate_compile(const std::string &source_code_path) { 

    Compile_Status compile_status;
    compile_status.binary = static_cast<std::string>("sol") + box_id;
    compile_status.binary_path = box_path + compile_status.binary;

    //compile using g++
    const auto box_id_init_arg = (std::string)"--box-id=" + box_id;
    std::string metadata_file_path = temp_dir + box_id + (std::string)".meta";
    std::string metadata_init = (std::string)"--meta=" + metadata_file_path;
    std::string stderr_arg = (std::string)"--stderr=err" + box_id + (std::string)".err";
    std::vector<char*> run_args = {
        (char*)"isolate",
        const_cast<char*>(box_id_init_arg.c_str()),
        (char*)"--run", 
        const_cast<char*>(metadata_init.c_str()), 
        const_cast<char*>(stderr_arg.c_str()), 
        (char*)"--cg",
        (char*)"--processes=64",
        (char*)"--env=PATH",
    };
    run_args.push_back((char*)"--");
    run_args.push_back(const_cast<char*>(CPP_COMPILER.c_str()));
    run_args.push_back(const_cast<char*>(source_code_path.c_str()));
    run_args.push_back((char*)"-o");
    run_args.push_back(const_cast<char*>(compile_status.binary.c_str()));
    for(const auto arg : isolate_compile_args) {
        run_args.push_back(arg);
    }
    run_args.push_back(NULL);
    compile_status.status = new_process(
        "/usr/local/bin/isolate", 
        run_args.data(),
        -1, -1, STDERR_FILENO
    ); 

    if(compile_status.status == Verdict::CHILD_PROCESS_ERROR) return compile_status;

    //check metadata verdict 
    compile_status.status = metadata_verdict(metadata_file_path);
    rm(metadata_file_path);
    return compile_status; 
}
