#include <iostream>
#include "isolate_utils.hpp"
#include "process_utils.hpp"


using namespace std;


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


//isAlphaNumeric
bool isAlphaNumeric(const string& s) {
    for (char c : s) {
        if (!isalnum(c)) return false;
    }
    return true;
}



//Judge function
int judge(const string& binary, const string& binary_path, const string& input_file, const string& input_file_path, const string& answer_file_path) { 
 
    //run
    auto [status, boxid] = isolate_run(binary, binary_path, input_file, input_file_path);
    if(status == CHILD_PROCESS_ERROR) {
        cerr<<"Unable to spawn new process: isolate sandbox\n"; 
        return status;
    } 

    //check the metadata verdict
    string metadata_file = (string)"metadata" + boxid + (string)".meta";
    string metadata_file_path = (string)"/tmp/" + metadata_file;
    status = metadata_verdict(metadata_file_path);
    rm(metadata_file_path);
    if(status == CHILD_PROCESS_ERROR) {
        cerr<<"Error opening file: "<<metadata_file_path<<'\n'; 
        return CHILD_PROCESS_ERROR;
    }
    error_msg(status);
    if(status) return status;

    string output_file = (string)"out" + boxid + ".txt";
    string output_file_path = (string)"/tmp/" + output_file;

    //No answer to check from
    if(answer_file_path.empty()) {
        //show output file
        char *out_args[] = {
            (char*)"cat",
            const_cast<char*>(output_file_path.c_str()),
            NULL
        };
        status = new_process("/usr/bin/cat", out_args, -1, -1);
        if(status == CHILD_PROCESS_ERROR) {
            cerr<<"Unable to spawn new process: cat\n";
        } 
        else if(status) {
            cerr<<"Output file not present\n";
        }

        //delete redundant files
        rm(output_file_path);  
        return status; 
    }

    //check the output and answer 
    char *diff_args[] = {
        (char*)"diff",
        const_cast<char*>(answer_file_path.c_str()),
        const_cast<char*>(output_file_path.c_str()),
        NULL
    };
    const int devnull = open("/dev/null", O_WRONLY);
    if(devnull == -1) {
        cerr<<"Unable to find /dev/null\n";
        return PROCESS_ERROR;
    }
    status = new_process("/usr/bin/diff", diff_args, -1, devnull); 
    close(devnull);
    rm(output_file_path); 
    if(status == CHILD_PROCESS_ERROR) {
        cerr<<"Unable to spawn new process: diff\n"; 
        return CHILD_PROCESS_ERROR;
    }
    if(!status) {
        return AC;
    }
    return WA; 
}


//run function
int run(const string& tc_path, const string& code) {
    
    //compile
    auto [compile_status, binary] = compile(code.c_str());
    if(compile_status == CHILD_PROCESS_ERROR) {
        cerr<<"Unable to spawn new process: g++\n";
        return CHILD_PROCESS_ERROR;
    }
    if(compile_status == PROCESS_ERROR) {
        cerr<<"Compilation Error\n";
        return compile_status;
    }
    string binary_path = (string)"./" + binary;
    error_msg(compile_status);
    if(compile_status) {
        rm(binary_path);
        return compile_status;
    }          
    
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
            rm(binary_path);
            if(ac == (i-1)) return AC;
            return WA;
        }
        if(access(answer_file_path.c_str(), F_OK) != 0) {
            cerr<<"Answer file "<<i<<" not present\n";
            i++;
            continue;
        }
        
        int status = judge(binary, binary_path, input_file, input_file_path, answer_file_path);
        
        if(status == CHILD_PROCESS_ERROR) {
            rm(binary_path);
            return CHILD_PROCESS_ERROR;
        }
        cout<<"Test "<<i<<": ";
        if(!status) {
            cout<<"Passed\n";
            ac++;
        }
        else {
            cout<<"Wrong Answer\n";
        }
        i++; 
    } 
    return 0;
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
            return PROCESS_ERROR;
        }
        
        string code = argv[2];
        
        //compile
        auto [status, binary] = compile(code.c_str());
        if(status == CHILD_PROCESS_ERROR) {
            cerr<<"Unable to spawn new process: g++\n";
            return CHILD_PROCESS_ERROR;
        }
        if(status == PROCESS_ERROR) {
            cerr<<"Compilation Error\n";
            return status;
        }
        string binary_path = (string)"./" + binary;
        error_msg(status);
        if(status) {
            rm(binary_path);
            return status;
        } 
        
        //take input
        ofstream input_file("./input.txt");
        string temp;
        while(getline(cin, temp)) {
            input_file << temp << '\n';
        }
        input_file.close(); 
        
        //judge without answer
        status = judge(binary, binary_path, "input.txt", "./input.txt", "");
        rm(binary_path);
        rm("./input.txt");
        return status;
    } 
    
    
    //run command
    if(cmd == "run") {
        if(argc != 5) {
            cout<<"Usage:\n";
            cout<<"LJS run <Lab number> <Problem number> <source code>\n";
            return 1;
        }
        string lab = (string)"Lab" + argv[2];
        string prob = (string)"q" + argv[3];
        if(!isAlphaNumeric(lab) || !isAlphaNumeric(prob)) {
            cerr<<"Lab Number and Problem Number must be AlphaNumeric\n";
            return 1;
        }
        
        
        string tc_path = (string)"./" + lab + (string)"/Problem/" + prob + (string)"/"; 
        
        return run(tc_path, argv[4]);
    }
    
    
    //submit
    if(cmd == "submit") {
        if(argc != 5) {
            cout<<"Usage:\n";
            cout<<"LJS submit <Lab number> <Problem number> <source code>\n";
            return 1;
        }
        string lab = (string)"Lab" + argv[2];
        string prob = (string)"q" + argv[3];
        if(!isAlphaNumeric(lab) || !isAlphaNumeric(prob)) {
            cerr<<"Lab Number and Problem Number must be AlphaNumeric\n";
            return 1;
        }

        string tc_ex_path = (string)"./" + lab + (string)"/Problem/" + prob + (string)"/";
        string tc_path = (string)"./" + lab + (string)"/Hidden/" + prob + (string)"/"; 
        
        //first check if ex_tc passes
        if(run(tc_ex_path, argv[4]) == WA) {
            cout<<"Example Test Case Failed\n";
            return WA;
        } 

        return run(tc_path, argv[4]);
    }

    return 0;
}
