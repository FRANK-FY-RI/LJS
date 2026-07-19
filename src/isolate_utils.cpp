#include "../include/isolate_utils.hpp"



//Isolate run
std::pair<int, std::string> isolate_run(const int cfd, const std::string& binary_file, const std::string& binary_file_path, const std::string& input_file, const std::string& input_file_path) {
    int curr_box = box_cnt.fetch_add(1) % 1000;
    std::string box_id = std::to_string(curr_box);
    std::string box_path = "/var/local/lib/isolate/" + box_id + "/box/";
    std::string box_id_init_arg = (std::string)"--box-id=" + box_id;

    //Initialize the box
    char *init_args[] = {
        (char*)"isolate",
        (char*)"--init",
        (char*)"--cg",
        const_cast<char*>(box_id_init_arg.c_str()),
        NULL
    };

    const int devnull = open("/dev/null", O_WRONLY);
    if(devnull == -1) {
        return {CHILD_PROCESS_ERROR, box_id};
    }
    int status = new_process("/usr/local/bin/isolate", init_args, -1, devnull, cfd);
    close(devnull);
    // cout<<"Init status: "<<status<<endl;
    if(status) return {status, box_id};

    //Populate
    namespace fs = std::filesystem;
    try {
        fs::copy_file(
            binary_file_path,
            fs::path(box_path) / fs::path(binary_file_path).filename(),
            fs::copy_options::overwrite_existing
        );

        fs::copy_file(
            input_file_path,
            fs::path(box_path) / fs::path(input_file_path).filename(),
            fs::copy_options::overwrite_existing
        );
    }
    catch (const fs::filesystem_error&) {
        isolate_cleanup(cfd, box_id);
        return {PROCESS_ERROR, box_id};
    }
 

    //run
    std::string output_file = (std::string)"out" + box_id + ".txt";
    std::string metadata_file_path = (std::string)"/tmp/metadata" + box_id + ".meta";
    std::string metadata_init = (std::string)"--meta=" + metadata_file_path;
    std::string stdin_arg = (std::string)"--stdin=" + input_file;
    std::string stdout_arg = (std::string)"--stdout=" + output_file;
    char *run_args[] = {
        (char*)"isolate",
        const_cast<char*>(box_id_init_arg.c_str()),
        (char*)"--run",
        (char*)"--time=2",
        (char*)"--cg",
        (char*)"--cg-mem=10240",
        const_cast<char*>(metadata_init.c_str()),
        const_cast<char*>(stdin_arg.c_str()),
        const_cast<char*>(stdout_arg.c_str()),
        (char*)"--",
        const_cast<char*>(binary_file.c_str()),
        NULL
    };
    status = new_process("/usr/local/bin/isolate", run_args, -1, -1, cfd); 
    if(status) {
        isolate_cleanup(cfd, box_id);    
        return {status, box_id};
    }

    //copy output file
    std::string output_file_source = box_path + output_file;
    std::string output_file_dest = (std::string)"/tmp/" + output_file;
    char *copy_out_args[] = {
        (char*)"cp",
        const_cast<char*>(output_file_source.c_str()),
        const_cast<char*>(output_file_dest.c_str()),
        NULL
    };
    status = new_process("/usr/bin/cp", copy_out_args, -1, -1, cfd);
    // cout<<"output copy status: "<<status<<endl;
 
    isolate_cleanup(cfd, box_id);
    return {status, box_id}; 
}



//Isolate cleanup
int isolate_cleanup(const int cfd, std::string boxid) {
    std::string box_init = "--box-id=" + boxid;
    char *args[] = {
        (char*)"isolate",
        (char*)"--cleanup",
        (char*)"--cg",
        const_cast<char*>(box_init.c_str()),
        NULL
    };
    return new_process("/usr/local/bin/isolate", args, -1, -1, cfd);
}



//metadata verdict
int metadata_verdict(const std::string& metadata_file_path) {
    std::ifstream file(metadata_file_path); 
    if(!file.is_open()) {
        return CHILD_PROCESS_ERROR;
    }
    std::string key, value, status;
    while (std::getline(file, key, ':') && std::getline(file, value)) {
        if(key == "cg-oom-killed") return MLE;
        else if(key == "status") status = value;
        else if(key == "exitsig") exitsig = value;
    }
    if(status == "TO") return TLE; 
    if(!exitsig.empty()) return RUNTIME_ERROR;
    return 0;
}