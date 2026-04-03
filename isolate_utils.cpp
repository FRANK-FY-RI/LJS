#include "isolate_utils.hpp"



//Isolate run
pair<int, string> isolate_run(const string& binary_file, const string& binary_file_path, const string& input_file, const string& input_file_path) {
    size_t pid = getpid();
    pid = pid%1000;
    string box_id = std::to_string(pid);
    string box_path = "/var/local/lib/isolate/" + box_id + "/box/";
    string box_id_init_arg = (string)"--box-id=" + box_id;

    //Initialize the box
    char *init_args[] = {
        (char*)"isolate",
        (char*)"--init",
        const_cast<char*>(box_id_init_arg.c_str()),
        NULL
    };

    int devnull = open("/dev/null", O_WRONLY);
    int status = new_process("/usr/local/bin/isolate", init_args, -1, devnull);
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
    status = new_process("/usr/bin/cp", bin_copy_args, -1, 2);
    // cout<<"binary copy status: "<<status<<endl;
    if(status) {
        isolate_cleanup(box_id);
        return {status, ""};
    }
    
        //input file 
    char *inp_copy_args[] = {
        (char*)"cp",
        const_cast<char*>(input_file_path.c_str()),
        const_cast<char*>(box_path.c_str()),
        NULL
    };
    status = new_process("/usr/bin/cp", inp_copy_args, -1, 2);
    // cout<<"input copy status: "<<status<<endl;
    if(status) {
        isolate_cleanup(box_id);    
        return {status, box_id}; 
    }

    //run
    string output_file = (string)"out" + box_id + ".txt";
    string metadata_file_path = (string)"/tmp/metadata" + box_id + ".meta";
    string metadata_init = (string)"--meta=" + metadata_file_path;
    string stdin_arg = (string)"--stdin=" + input_file;
    string stdout_arg = (string)"--stdout=" + output_file;
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
    status = new_process("/usr/local/bin/isolate", run_args, -1, 2);
    // cout<<"run status: "<<status<<endl;
    if(status) {
        isolate_cleanup(box_id);    
        return {status, box_id};
    }

    //copy output file
    string output_file_source = box_path + output_file;
    string output_file_dest = (string)"/tmp/" + output_file;
    char *copy_out_args[] = {
        (char*)"cp",
        const_cast<char*>(output_file_source.c_str()),
        const_cast<char*>(output_file_dest.c_str()),
        NULL
    };
    status = new_process("/usr/bin/cp", copy_out_args, -1, 2);
    // cout<<"output copy status: "<<status<<endl;
 
    isolate_cleanup(box_id);
    return {status, box_id}; 
}



//Isolate cleanup
int isolate_cleanup(string boxid) {
    string box_init = "--box-id=" + boxid;
    char *args[] = {
        (char*)"isolate",
        (char*)"--cleanup",
        const_cast<char*>(box_init.c_str()),
        NULL
    };
    return new_process("/usr/local/bin/isolate", args, -1, 2);
}



//metadata verdict
int metadata_verdict(const string& metadata_file_path) {
    std::ifstream file(metadata_file_path); 
    if(!file.is_open()) {
        return CHILD_PROCESS_ERROR;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    string metadata_file = buffer.str();
    // cout<<"\n\nMetadata file contains: "<<metadata_file<<"\n\n";
    int index = metadata_file.find("status:");
    if(index == string::npos) return 0;
    index += 7;
    const string verdict_s = metadata_file.substr(index, 2);
    if(verdict_s == "TO") return TLE;
    else if(verdict_s == "MO") return MLE;
    else return RUNTIME_ERROR; 
}