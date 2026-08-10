#ifndef __CONFIG
#define __CONFIG


#include <vector>
#include <string>

//Time Limit for program execution in seconds
inline const std::string Time_Limit = "2";

//Memory Limit for program execution in Kilobytes(KB)
inline const std::string Memory_Limit = "1000";

//g++ compiler flags
const std::vector<char*> compiler_args = {
    (char*)"-std=c++20",
    (char*)"-Wall",
    (char*)"-static-libstdc++"
};

//Server's socket address
constexpr char SV_SOCK_ADDR[] = "/tmp/sv_sock_addr";

//Maximum number of worker threads in the threadpool
constexpr int max_threads = 11;

//Address for keeping temporary files
const std::string temp_dir = "/tmp/";

#endif