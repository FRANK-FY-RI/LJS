#include "isolate_utils.hpp"



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
        //binary file
    char *bin_copy_args[] = {
        (char*)"cp",
        const_cast<char*>(binary_file_path.c_str()),
        const_cast<char*>(box_path.c_str()),
        NULL
    };
    status = new_process("/usr/bin/cp", bin_copy_args, -1, -1, cfd);
    // cout<<"binary copy status: "<<status<<endl;
    if(status) {
        isolate_cleanup(cfd, box_id);
        return {status, box_id};
    }
    
        //input file 
    char *inp_copy_args[] = {
        (char*)"cp",
        const_cast<char*>(input_file_path.c_str()),
        const_cast<char*>(box_path.c_str()),
        NULL
    };
    status = new_process("/usr/bin/cp", inp_copy_args, -1, -1, cfd);
    // cout<<"input copy status: "<<status<<endl;
    if(status) {
        isolate_cleanup(cfd, box_id);    
        return {status, box_id}; 
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
        (char*)"--mem=10240",
        const_cast<char*>(metadata_init.c_str()),
        const_cast<char*>(stdin_arg.c_str()),
        const_cast<char*>(stdout_arg.c_str()),
        (char*)"--",
        const_cast<char*>(binary_file.c_str()),
        NULL
    };
    status = new_process("/usr/local/bin/isolate", run_args, -1, -1, cfd);
    // cout<<"run status: "<<status<<endl;
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
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string metadata_file = buffer.str();
    file.close();
    // cout<<"\n\nMetadata file contains: "<<metadata_file<<"\n\n";
    int index = metadata_file.find("status:");
    if(index == std::string::npos) return 0;
    index += 7;
    const std::string verdict_s = metadata_file.substr(index, 2);
    if(verdict_s == "TO") return TLE;
    else if(verdict_s == "MO") return MLE;
    return RUNTIME_ERROR; 
}