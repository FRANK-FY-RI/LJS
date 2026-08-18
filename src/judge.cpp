#include "../include/judge.hpp"


// check source code from client's directory
std::optional<std::string> resolve_source(
    const std::string& cwd,
    const std::string& filename,
    uid_t uid
) {
    struct passwd* pw = getpwuid(uid);

    if (!pw)
        return std::nullopt;

    std::filesystem::path home = pw->pw_dir;

    std::error_code ec;

    std::filesystem::path source =
        std::filesystem::weakly_canonical(
            std::filesystem::path(cwd) / filename,
            ec
        );

    if (ec)
        return std::nullopt;

    auto relative =
        std::filesystem::relative(source, home, ec);

    if (ec || relative.empty() ||
        *relative.begin() == "..") {

        return std::nullopt;
    }

    struct stat st{};

    if (stat(source.c_str(), &st) == -1)
        return std::nullopt;

    if (st.st_uid != uid)
        return std::nullopt;

    return source.string();
}

//judge function
int judge(
    int cfd,
    const std::string& binary_file,
    const std::string& binary_file_path,
    const std::string& input_file,
    const std::string& input_file_path,
    const std::string& answer_file_path
) { 
    
    //Initialize the sandbox
    Isolate_Init_status isolate_init_status = isolate_init();

    //Populate the sandbox
    //copy the binary into the sandbox
    if(copy_file(binary_file_path, isolate_init_status.box_path)) {
        send_client(cfd, "Unable to copy binary file into the sandbox\n");
        return PROCESS_ERROR;    
    }
    //copy the input file into the sandbox
    if(copy_file(input_file_path, isolate_init_status.box_path)) {
        send_client(cfd, "Unable to copy input file into the sandbox\n");
        return PROCESS_ERROR;
    }
 
    //run
    auto isolate_run_status = isolate_run(isolate_init_status.box_id, binary_file, input_file);
    if(isolate_run_status == CHILD_PROCESS_ERROR) {
        send_client(cfd, "Unable to spawn new process: isolate sandbox\n");
        return isolate_run_status;
    }  

    //check the metadata verdict
    std::string metadata_file = isolate_init_status.box_id + (std::string)".meta";
    std::string metadata_file_path = temp_dir + metadata_file;
    auto metadata_status = metadata_verdict(metadata_file_path);
    rm(metadata_file_path);
    if(metadata_status == PROCESS_ERROR) {
        send_client(cfd, "Unable to open metadata file\n");
        return PROCESS_ERROR;
    }
    if(metadata_status) return metadata_status;

    //copy the output file into temporary directory
    const std::string output_file = (std::string)"out" + isolate_init_status.box_id + ".txt";
    const std::string output_file_path = temp_dir + output_file;
    if(copy_file(
        isolate_init_status.box_path + output_file,
        temp_dir
    )) {
        send_client(cfd, "Unable to copy output file\n");
        return PROCESS_ERROR;
    } 

    //check the output and answer  
    auto diff_status = diff(answer_file_path, output_file_path); 
    rm(output_file_path); 
    if(diff_status == PROCESS_ERROR) {
        send_client(cfd, "Unable to open output or answer files\n");
        return PROCESS_ERROR;
    } 
    else if(diff_status == 0) return AC;
    return WA;
}



//run function
int runfn(int cfd, const std::string& tc_path, const std::string& code) {
    
    //compile
    auto compile_status = compile(cfd, code.c_str());
    if(compile_status.status == CHILD_PROCESS_ERROR) {
        send_client(cfd, "Unable to spawn new process: g++\n");
        return CHILD_PROCESS_ERROR;
    } 
    else if(compile_status.status == TLE) {
        send_client(cfd, "Compilation Time Limit Exceeded\n");
        return TLE;
    }
    else if(compile_status.status) {
        send_client(cfd, "Compilation error\n");
        return compile_status.status;
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
            std::string verdict;

            if (ac == i - 1) {
                verdict =
                    "\033[1;32m────────────────────────\033[0m\n"
                    "\033[1;32m✔ Accepted — " +
                    std::to_string(ac) + "/" + std::to_string(i - 1) +
                    " Passed\033[0m\n"
                    "\033[1;32m────────────────────────\033[0m\n";
            }
            else {
                verdict =
                    "\033[1;31m────────────────────────\033[0m\n"
                    "\033[1;31m✘ Wrong Answer — " +
                    std::to_string(ac) + "/" + std::to_string(i - 1) +
                    " Passed\033[0m\n"
                    "\033[1;31m────────────────────────\033[0m\n";
            }

            send_client(cfd, verdict);
            rm(compile_status.binary_path);
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
        
        int status = judge(
            cfd,
            compile_status.binary,
            compile_status.binary_path,
            input_file,
            input_file_path,
            answer_file_path
        );
         
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
int run(int cfd, std::vector<std::string> &argv, const std::string& client_cwd, uid_t client_uid) { 
    std::string lab = (std::string)"Lab" + argv[1];
    std::string prob = (std::string)"prob_" + argv[2]; 
    std::string tc_path = prob_dir + lab + (std::string)"/Problem/" + prob + (std::string)"/"; 
    auto source = resolve_source(client_cwd, argv[3], client_uid);
    if(!source) {
        send_client(cfd, "Invalid source file\n");
        return PROCESS_ERROR;
    }
    return runfn(cfd, tc_path, *source);
}


//submit
int submit(int cfd, std::vector<std::string> &argv, const std::string& client_cwd, uid_t client_uid) { 
    std::string lab = (std::string)"Lab" + argv[1];
    std::string prob = (std::string)"prob_" + argv[2]; 
    std::string tc_ex_path = prob_dir + lab + (std::string)"/Problem/" + prob + (std::string)"/";
    std::string tc_path = prob_dir + lab + (std::string)"/Hidden/" + prob + (std::string)"/"; 
    auto source = resolve_source(client_cwd, argv[3], client_uid);
    if(!source) {
        send_client(cfd, "Invalid source file\n");
        return PROCESS_ERROR;
    }
    
    //first check if ex_tc passes
    if(runfn(cfd, tc_ex_path, *source) != AC) {
        if(send_client(cfd, "Example Test Case Failed\n")==-1) return PROCESS_ERROR;
        return WA;
    } 
    return runfn(cfd, tc_path, *source);
}