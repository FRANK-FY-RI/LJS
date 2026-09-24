#include "../include/judge.hpp"
#include "../include/isolate_utils.hpp"
#include <filesystem>
#include <sstream>
#include <string>
#include <cstdint>
#include <fstream>


// check source code from client's directory
std::optional<std::string> resolve_source(
    ClientContext& client,
    const std::string& filename
) {
    struct passwd* pw = getpwuid(client.uid);

    if (!pw)
        return std::nullopt;

    client.username = pw->pw_name;

    std::filesystem::path home = pw->pw_dir;

    std::error_code ec;

    std::filesystem::path source =
        std::filesystem::weakly_canonical(
            std::filesystem::path(client.cwd) / filename,
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

    if (st.st_uid != client.uid)
        return std::nullopt;

    return source.string();
}


//Hashing Function
std::string hash(const std::string& source_code) {
    constexpr uint64_t B1 = 131;
    constexpr uint64_t B2 = 137;

    uint64_t h1 = 0;
    uint64_t h2 = 0;

    for (const unsigned char c : source_code) {
        h1 = h1 * B1 + c;
        h2 = h2 * B2 + c;
    }
    
    std::string HASH = std::to_string(h1) + std::to_string(h2);
    return HASH;
}


//judge function
Verdict judge(
    int cfd,
    const std::string& binary_file,
    const std::string& binary_file_path,
    const std::string& input_file,
    const std::string& input_file_path,
    const std::string& answer_file_path
) { 
    
    //Initialize the sandbox
    Isolate_Init_status isolate_init_status = isolate_init();
    if(isolate_init_status.status != Verdict::SUCCESS) {
        send_client(cfd, "Unable to Initialize a new sandbox\n");
        return Verdict::PROCESS_ERROR;
    }

    //Populate the sandbox
    //copy the binary into the sandbox
    if(copy_file(binary_file_path, isolate_init_status.box_path) != Verdict::SUCCESS) {
        send_client(cfd, "Unable to copy binary file into the sandbox\n");
        if(isolate_cleanup(isolate_init_status.box_id) != Verdict::SUCCESS) {
            send_client(cfd, "Unable to delete the sandbox\n");
        }
        return Verdict::PROCESS_ERROR;    
    }
    //copy the input file into the sandbox
    if(copy_file(input_file_path, isolate_init_status.box_path) != Verdict::SUCCESS) {
        send_client(cfd, "Unable to copy input file into the sandbox\n");
        if(isolate_cleanup(isolate_init_status.box_id) != Verdict::SUCCESS) {
            send_client(cfd, "Unable to delete the sandbox\n");
        }
        return Verdict::PROCESS_ERROR;
    }
 
    //run
    auto isolate_run_status = isolate_run(isolate_init_status.box_id, binary_file, input_file);
    if(isolate_run_status == Verdict::CHILD_PROCESS_ERROR) {
        send_client(cfd, "Unable to run the sandbox\n");
        if(isolate_cleanup(isolate_init_status.box_id) != Verdict::SUCCESS) {
            send_client(cfd, "Unable to delete the sandbox\n");
        } 
        return isolate_run_status;
    }  

    //check the metadata verdict
    std::string metadata_file = isolate_init_status.box_id + (std::string)".meta";
    std::string metadata_file_path = temp_dir + metadata_file;
    auto metadata_status = metadata_verdict(metadata_file_path);
    rm(metadata_file_path);
    if(metadata_status == Verdict::PROCESS_ERROR) {
        send_client(cfd, "Unable to open metadata file\n");
        if(isolate_cleanup(isolate_init_status.box_id) != Verdict::SUCCESS) {
            send_client(cfd, "Unable to delete the sandbox\n");
        }
        return Verdict::PROCESS_ERROR;
    }

    //send the error to the client
    if(metadata_status != Verdict::SUCCESS) {
        const std::string error_file = (std::string)"err" + isolate_init_status.box_id + ".err"; 
        const std::string error_file_path = isolate_init_status.box_path + error_file;
 
        {
            std::ifstream file(error_file_path);
            if(!file) return metadata_status;
            std::string content(
                    (std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>()
                    );

            if (!content.empty()) {
                send_client(cfd, content);
            }
        }

        if(isolate_cleanup(isolate_init_status.box_id) != Verdict::SUCCESS) {
            send_client(cfd, "Unable to delete the sandbox\n");
        }   

        return metadata_status;
    }

    //copy the output file into temporary directory
    const std::string output_file = (std::string)"out" + isolate_init_status.box_id + ".txt";
    const std::string output_file_path = isolate_init_status.box_path + output_file; 

    //check the output and answer  
    auto diff_status = diff(answer_file_path, output_file_path); 

    if(isolate_cleanup(isolate_init_status.box_id) != Verdict::SUCCESS) {
        send_client(cfd, "Unable to delete the sandbox\n");
    }
    if(diff_status == Verdict::PROCESS_ERROR) {
        send_client(cfd, "Unable to open output or answer files\n");
        return Verdict::PROCESS_ERROR;
    } 
    else if(diff_status == Verdict::SUCCESS) return Verdict::AC;
    return Verdict::WA;
}



//run function
Verdict runfn(int cfd, const std::string& tc_path, const std::string& code) {
    
    //compile
    auto compile_status = compile(cfd, code.c_str());
    if(compile_status.status == Verdict::CHILD_PROCESS_ERROR) {
        send_client(cfd, "Unable to spawn new process: g++\n");
        return Verdict::CHILD_PROCESS_ERROR;
    } 
    else if(compile_status.status == Verdict::TLE) {
        send_client(cfd, "Compilation Time Limit Exceeded\n");
        return Verdict::TLE;
    }
    else if(compile_status.status != Verdict::SUCCESS) {
        send_client(cfd, "Compilation error\n");
        return compile_status.status;
    }          
    
    int i = 1;
    int ac = 0;
    Verdict final_verdict = Verdict::AC;
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
            return final_verdict; 
        }
        if(access(answer_file_path.c_str(), F_OK) != 0) {
            std::string msg = static_cast<std::string>("Answer file ") + 
            std::to_string(i) + static_cast<std::string>(" not present\n");
            if(send_client(cfd, msg) == Verdict::CONNECTION_ERROR) return Verdict::CONNECTION_ERROR;
            i++;
            continue;
        }
        
        auto status = judge(
            cfd,
            compile_status.binary,
            compile_status.binary_path,
            input_file,
            input_file_path,
            answer_file_path
        );
         
        std::string msg = static_cast<std::string>("Test ") + 
        std::to_string(i) + static_cast<std::string>(": ");
        if(send_client(cfd, msg) == Verdict::CONNECTION_ERROR) {
            rm(compile_status.binary_path);
            return Verdict::PROCESS_ERROR;
        }
        if(status == Verdict::AC) ac++;
        else if(final_verdict == Verdict::AC) final_verdict = status; 
        error_msg(cfd, status); 
        i++; 
    } 
    return Verdict::SUCCESS;
}


//run command
Verdict run(ClientContext& client, std::vector<std::string> &argv) { 
    std::string lab = (std::string)"Lab" + argv[1];
    std::string prob = (std::string)"prob_" + argv[2]; 
    std::string tc_path = prob_dir + lab + (std::string)"/Problem/" + prob + (std::string)"/"; 
    auto source = resolve_source(client, argv[3]);
    if(!source) {
        send_client(client.cfd, "Invalid source file\n");
        return Verdict::PROCESS_ERROR;
    }
    return runfn(client.cfd, tc_path, *source);
}


//submit
Verdict submit(ClientContext& client, std::vector<std::string> &argv, const LJSdatabase& database) { 
    std::string lab = (std::string)"Lab" + argv[1];
    std::string prob = (std::string)"prob_" + argv[2]; 
    std::string tc_ex_path = prob_dir + lab + (std::string)"/Problem/" + prob + (std::string)"/";
    std::string tc_path = prob_dir + lab + (std::string)"/Hidden/" + prob + (std::string)"/"; 
    auto source = resolve_source(client, argv[3]);
    if(!source) {
        send_client(client.cfd, "Invalid source file\n");
        return Verdict::PROCESS_ERROR;
    }
    
    //first check if ex_tc passes
    Verdict verdict = runfn(client.cfd, tc_ex_path, *source);
    if(verdict == Verdict::AC) verdict = runfn(client.cfd, tc_path, *source); 

    std::ifstream user_code(*source); 
    std::stringstream buffer;
    buffer << user_code.rdbuf();
    const std::string code_id = hash(buffer.str());

    namespace fs = std::filesystem;
    fs::path source_fs(*source);
    const std::string code_storage_path = persistent_code_dir + code_id + ".cpp";
    fs::path destination_fs(code_storage_path);

    try {
        fs::create_directories(destination_fs.parent_path());
        fs::copy_file(
            *source,
            code_storage_path,
            fs::copy_options::overwrite_existing
        );
    }
    catch(const fs::filesystem_error& e) {}

    database.submissions.insert_row({
            {"user_id", client.username},
            {"code_id", code_id},
            {"verdict", verdict_to_string(verdict)}
    });
    database.codes.insert_row({
            {"code_id", code_id},
            {"code", code_storage_path}
    });
    return verdict;
}
