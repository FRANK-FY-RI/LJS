#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>

#define COMPILE_ERROR 1
#define RUNTIME_ERROR 2
#define TLE 3
#define CHILD_PROCESS_ERROR 4

using namespace std;

//fucntion to start a new process
int new_process(const char* path, char *args[]) {
    int status;
    pid_t pid = fork();
    int ret_status;
    if(pid == 0) {
        execv(path, args);
        exit(CHILD_PROCESS_ERROR);
    }
    else {
        waitpid(pid, &status, 0);
        if(WIFEXITED(status)) {
            ret_status = WEXITSTATUS(status);
        }
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
    int status = new_process("/usr/bin/g++", compile_args); 
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
        status = new_process(exec_path.c_str(), run_args); 
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
            string exec_path = "./" + binary;
            char *run_args[] = {
                const_cast<char*>(binary.c_str()),

            }
            int run_status = new_process(exec_path.c_str(), );
            if(status == COMPILE_ERROR) {
                return COMPILE_ERROR;
            }
            else if(status == CHILD_PROCESS_ERROR) {
                return CHILD_PROCESS_ERROR;
            }
            else if(status == RUNTIME_ERROR) {
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
            status = new_process("/usr/bin/diff", diff_args); 
            if(status == CHILD_PROCESS_ERROR) {
                cerr<<"⚠️ Unable to run diff command\n";
            }
            else if(status == 0) {ac++;}
            i++;
        } 
    }
    return 0;
}