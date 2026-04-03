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

        //check for metadata verdict
        string metadata_file = (string)"metadata" + boxid + ".meta";
        string metadata_file_path = (string)"/tmp/" + metadata_file;
        status = metadata_verdict(metadata_file_path);
        // cout<<"metadata verdict"<<status<<endl;
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
                break;
            }
            
            //run
            auto [status, boxid] = isolate_run(binary, binary_path, input_file, input_file_path);
            error_msg(status);
            if(status == CHILD_PROCESS_ERROR) return status;
            if(status) {i++; continue;} 

            //check the output and answer
            string output_file = (string)"out" + boxid + ".txt";
            string output_file_path = "/tmp/" + output_file;
            string metadata_file = (string)"metadata" + boxid + (string)".meta";
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
