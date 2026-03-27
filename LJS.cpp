#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define COMPILE_ERROR 1
#define RUNTIME_ERROR 2
#define TLE 3
#define CHILD_PROCESS_ERROR 4

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

        execv(path, args);
        exit(CHILD_PROCESS_ERROR);
    }
    else {
        waitpid(pid, &status, 0);
        if(WIFEXITED(status)) {
            ret_status = WEXITSTATUS(status);
        }
        else if(WIFSIGNALED(status)) ret_status = RUNTIME_ERROR;
        else ret_status = CHILD_PROCESS_ERROR;
    }
    return ret_status;
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
        NULL
    };
    int status = new_process("/usr/bin/g++", compile_args, -1, -1); 
    if(status == COMPILE_ERROR) {
        cerr<<"❌ Compilation error\n";
        return {COMPILE_ERROR, ""};
    }
    else if(status == CHILD_PROCESS_ERROR) {
        cerr<<"⚠️ Unable to run g++ compiler\n";
        return {CHILD_PROCESS_ERROR, ""};
    }
    return {0, binary};
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
        string code = argv[2];
        auto [status, binary] = compile(code.c_str());
        if(status == COMPILE_ERROR) return COMPILE_ERROR;
        else if(status == CHILD_PROCESS_ERROR) return CHILD_PROCESS_ERROR;
        string exec_path = "./" + binary;     
        char *run_args[] = {
            const_cast<char*>(exec_path.c_str()), 
            NULL
        };
        status = new_process(exec_path.c_str(), run_args, -1, -1); 
        if(status == RUNTIME_ERROR) {
            cerr<<"⚠️ Runtime error\n";
            return RUNTIME_ERROR;
        }
        else if(status == CHILD_PROCESS_ERROR) {
            cerr<<"⚠️ Unable to run the binary\n";
            return CHILD_PROCESS_ERROR;
        }
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
        string tc_path = "/home/sb/" + lab + "/Problem" + prob + "/";
        auto [compile_status, binary] = compile(code.c_str());
        if(compile_status == COMPILE_ERROR) return COMPILE_ERROR;
        else if(compile_status == CHILD_PROCESS_ERROR) return CHILD_PROCESS_ERROR;
        int i = 1;
        int ac = 0;
        while(true) {
            string input_file = tc_path + to_string(i) + ".in";
            string answer_file = tc_path + to_string(i) + ".ans";
            string output_file = to_string(i) + ".out";
            
            //Check if file exists
            bool file_exists = false;
            if(access(input_file.c_str(), F_OK) == 0 ) file_exists = true;
            if(!file_exists) {
                if(ac == i-1) cout<<"✅ ";
                else cout<<"❌ ";
                cout<<ac<<"/"<<i-1<<" Passed\n";
                return 0;
            }
            
            //run
            const int input_fd = open(input_file.c_str(), O_RDONLY);
            const int output_fd = open(output_file.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
            string exec_path = "./" + binary;
            char *run_args[] = {
                const_cast<char*>(binary.c_str()),
                NULL
            };
            int status = new_process(exec_path.c_str(), run_args, input_fd, output_fd);
            close(input_fd);
            close(output_fd);
            if(status == COMPILE_ERROR) {
                return COMPILE_ERROR;
            }
            else if(status == CHILD_PROCESS_ERROR) {
                return CHILD_PROCESS_ERROR;
            }
            else if(status == RUNTIME_ERROR) {
                cerr<<"❌ Runtime Error";
                i++;
                continue;
            }

            //check the output and answer
            char *diff_args[] = {
                (char*)"diff",
                const_cast<char*>(answer_file.c_str()),
                const_cast<char*>(output_file.c_str()),
                NULL
            };
            status = new_process("/usr/bin/diff", diff_args, -1, 2); 
            if(status == CHILD_PROCESS_ERROR) {
                cerr<<"⚠️ Unable to run diff command\n";
                return CHILD_PROCESS_ERROR;
            }
            else if(status == 0) {
                cerr<<"✔️ Accepted\n";
                ac++;
            }
            else if(status == 1) {
                cerr<<"❌ Wrong Ans\n";
            }
            i++;
        } 
    }
    return 0;
}
