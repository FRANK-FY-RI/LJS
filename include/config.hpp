#ifndef __CONFIG
#define __CONFIG


#include <vector>
#include <sys/resource.h>
#include <chrono>
#include <string>

//Time Limit for runtime of program execution in seconds
inline const std::string Time_Limit = "2";

//Time Limit for program lifetime in seconds
inline const std::string Wall_Time_Limit = "5";

//Memory Limit for program execution in Kilobytes(KB)
inline const std::string Memory_Limit = "1000";

//Isolate Sandbox Run arguments
const std::string runtime_arg = (std::string)"--time=" + Time_Limit;
const std::string memory_arg = (std::string)"--cg-mem=" + Memory_Limit;
const std::string walltime_arg = static_cast<std::string>("--wall-time=") + Wall_Time_Limit;
const std::vector<char*> isolate_run_args = {
    const_cast<char*>(walltime_arg.c_str()),
    const_cast<char*>(runtime_arg.c_str()),
    const_cast<char*>(memory_arg.c_str()),
};

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

// Problem Directory
const std::string prob_dir = "/home/frank/sb/";

// Maximum length of path
constexpr size_t MAX_PATH = 10000;


// Common process limits
constexpr rlim_t COMMON_CPU_LIMIT = 60;
constexpr rlim_t COMMON_MEMORY_LIMIT = 2ULL * 1024 * 1024 * 1024; // 2 GB
constexpr rlim_t COMMON_FILE_SIZE_LIMIT = 512ULL * 1024 * 1024;   // 512 MB
constexpr rlim_t COMMON_NO_OF_FILE_LIMIT = 128;

constexpr std::chrono::seconds COMMON_WALL_TIMEOUT_SEC{15};

// Compiler-specific
constexpr rlim_t COMPILE_CPU_LIMIT = 10;
constexpr rlim_t COMPILE_MEMORY_LIMIT = 1ULL * 1024 * 1024 * 1024;
constexpr rlim_t COMPILE_FILE_SIZE_LIMIT = 16ULL * 1024 * 1024;
constexpr std::chrono::seconds COMPILE_WALL_TIMEOUT_SEC{10};


#endif