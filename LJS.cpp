#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define PROCESS_ERROR 1
#define RUNTIME_ERROR 2
#define TLE 3
#define MLE 4
#define CHILD_PROCESS_ERROR 5

using namespace std;

//fucntion to start a new process
int new_process(const char* path, char *args[], const int input_fd, const int output_fd) {
    int status;
    pid_t pid = fork();
    int ret_status;
    if(pid == 0) {
        //redirect input
        if(input_fd != -1) {
            dup2(input_fd, 0);
            close(input_fd);
        }

        //redirect output
        if(output_fd != -1) {
            dup2(output_fd, 1);
            close(output_fd);
        }

        struct rlimit rl;
        rl.rlim_cur = 2;
        rl.rlim_max = 3;
        setrlimit(RLIMIT_CPU, &rl);

        execv(path, args);
        exit(CHILD_PROCESS_ERROR);
    }
    else {
        waitpid(pid, &status, 0);
        if(WIFEXITED(status)) {
            ret_status = WEXITSTATUS(status);
        }
        else if(WIFSIGNALED(status)) {
            if(WTERMSIG(status) == SIGXCPU || WTERMSIG(status) == SIGALRM) ret_status = TLE;
            else ret_status = RUNTIME_ERROR;
        }
        else ret_status = CHILD_PROCESS_ERROR;
    }
    return ret_status;
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


//Isolate run
pair<int, string> isolate_run(const string& binary_file, const string& binary_file_path, const string& input_file, const string& input_file_path) {
    size_t pid = getpid();
    pid = pid%1000;
    string box_id = to_string(pid);
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


//Compile function
pair<int, string> compile(const char *code) {
    const string binary = "sol";
    char *compile_args[] = {
        (char*)"g++",
        (char*)"-std=c++23",
        (char*)"-Wall",
        const_cast<char*>(code),
        (char*)("-o"),
        const_cast<char*>(binary.c_str()),
        (char*)"-static-libstdc++",
        NULL
    };
    int status = new_process("/usr/bin/g++", compile_args, -1, -1); 
    return {status, binary};
}


//delete a file
inline int rm(const string& path) {
    return unlink(path.c_str()); 
}


//error message
void error_msg(int status) {
    if(status == CHILD_PROCESS_ERROR) {
        cerr<<"Unable to run some program\n";
    }
    else if(status == TLE) {
        cerr<<"Time Limit Exceeded\n";
    }
    else if(status == MLE) {
        cerr<<"Memory Limit Exceeded\n";
    }
    else if(status == RUNTIME_ERROR) {
        cerr<<"Runtime Error\n";
    }
    else if(status == PROCESS_ERROR) {
        cerr<<"Process Error\n";
    }
}


//metadata verdict
int metadata_verdict(const string& metadata_file_path) {
    ifstream file(metadata_file_path); 
    stringstream buffer;
    buffer << file.rdbuf();
    string metadata_file = buffer.str();
    int index = metadata_file.find("status:");
    if(index == string::npos) return 0;
    index += 7;
    const string verdict_s = metadata_file.substr(index, 2);
    if(verdict_s == "TO") return TLE;
    else if(verdict_s == "MO") return MLE;
    else return RUNTIME_ERROR; 
}



int main(int argc, char* argv[]) {
    //Help
    if(argc == 1 || string(argv[1]) == "--help") {
        cout << "LJS — Lab Judge System\n";
        cout << "                     -By Omkar Singh\n\n";
        cout << "Usage:\n";
        cout << "  LJS <option> <Lab number> <problem number> <source code>\n";
        cout << "   options are:\n";
        cout << "       run\n";
        cout << "       custom_run\n";
        cout << "       submit\n";
        cout << "Examples:\n";
        cout << "  LJS run 1 1 prob_1.cpp\n";
        return 0;
    }
    
    string cmd = argv[1];  
    
    //custom_run
    if(cmd == "custom_run") {
        if(argc != 3) {
            cerr<<"Usage:\n";
            cerr<<"LJS custom_run <source code>\n";
            return 1;
        }

        //compile
        string code = argv[2];
        auto [status, binary] = compile(code.c_str());
        if(status == PROCESS_ERROR) {
            cerr<<"Compilation Error\n";
            return status;
        }
        error_msg(status);
        if(status) return status; 

        //take input
        ofstream input_file("./input.txt");
        string temp;
        while(getline(cin, temp)) {
            input_file << temp << '\n';
        }
        input_file.close();

        //run
        string binary_path = (string)"./" + binary;
        auto [run_stat, boxid] = isolate_run(binary, binary_path, "input.txt", "./input.txt"); 
        status = run_stat;
        if(status == CHILD_PROCESS_ERROR) {
            cerr<<"Error running isolate sandbox\n";
            return CHILD_PROCESS_ERROR;
        }
        error_msg(status);
        if(status) return status;

        //check for metadata verdict
        string metadata_file = (string)"meta" + boxid + ".meta";
        string metadata_file_path = (string)"/tmp/" + metadata_file;
        status = metadata_verdict(metadata_file_path);
        error_msg(status);
        if(status) return status;

        //copy output file
        string output_file = (string)"out" + boxid + ".txt";
        string output_file_path = (string)"/tmp/" + output_file;
        char *out_args[] = {
            (char*)"cat",
            const_cast<char*>(output_file_path.c_str()),
            NULL
        };
        status = new_process("/usr/bin/cat", out_args, -1, -1);
        if(status == CHILD_PROCESS_ERROR) {
            cerr<<"Unable to show output file\n";
            return CHILD_PROCESS_ERROR;
        } 
        if(status) {
            cerr<<"Output file not present\n";
            return status;
        }

        //delete redundant files
        rm(output_file_path); 
        rm("./input.txt"); 
        rm(metadata_file_path);
        rm(binary_path);
        return 0;
    } 
    
    //run command
    if(cmd == "run") {
        if(argc != 5) {
            cout<<"Usage:\n";
            cout<<"LJS run <Lab number> <Problem number> <source code>\n";
            return 1;
        }
        string lab = "Lab" + string(argv[2]);
        string prob = "q" + string(argv[3]);
        string code = argv[4];
        string tc_path = "./" + lab + "/Problem/" + prob + "/"; 
        auto [compile_status, binary] = compile(code.c_str()); 
        error_msg(compile_status);
        if(compile_status) return compile_status; 
        string binary_path = (string)"./" + binary;
        int i = 1;
        int ac = 0;
        while(true) {
            string input_file = to_string(i) + ".in";
            string input_file_path = tc_path + input_file;
            string answer_file = to_string(i) + ".ans";
            string answer_file_path = tc_path + answer_file;
            
            //Check if file exists
            bool file_exists = false;
            if(access(input_file_path.c_str(), F_OK) == 0 ) file_exists = true;
            if(!file_exists) {
                if(ac == i-1) cout<<"✅ ";
                else cout<<"❌ ";
                cout<<ac<<"/"<<i-1<<" Passed\n";
                return 0;
            }
            
            //run
            auto [status, boxid] = isolate_run(binary, binary_path, input_file, input_file_path);
            error_msg(status);
            if(status == CHILD_PROCESS_ERROR) return status;
            if(status) {i++; continue;} 

            //check the output and answer
            string output_file = (string)"out" + boxid + ".txt";
            string output_file_path = "/tmp/" + output_file;
            string metadata_file = (string)"meta" + boxid + (string)".meta";
            string metadata_file_path = (string)"/tmp/" + metadata_file;
            char *diff_args[] = {
                (char*)"diff",
                const_cast<char*>(answer_file_path.c_str()),
                const_cast<char*>(output_file_path.c_str()),
                NULL
            };
            const int devnull = open("/dev/null", O_WRONLY);
            status = new_process("/usr/bin/diff", diff_args, -1, devnull); 
            rm(output_file_path); 
            error_msg(status); 
            if(status == CHILD_PROCESS_ERROR) return CHILD_PROCESS_ERROR;
            if(!status) {
                cout<<"Passed\n";
                ac++;
            }
            else {
                cout<<"Wrong Answer\n";
            }
            rm(output_file_path);
            rm(metadata_file_path);
            i++;
        } 
        rm(binary_path);
    }
    return 0;
}
