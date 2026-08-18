#ifndef __PROCESS_UTILS_HPP
#define __PROCESS_UTILS_HPP

#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <utility>
#include <string>
#include <fstream>
#include <vector>
#include <thread>
#include <filesystem>
#include "config.hpp"



#define AC 0
#define WA 1
#define RUNTIME_ERROR 2
#define TLE 3
#define MLE 4
#define CHILD_PROCESS_ERROR 5
#define PROCESS_ERROR 6


inline thread_local std::string exitsig;


// Process Limits
struct ProcessLimits {
    rlim_t cpu;
    rlim_t memory;
    rlim_t file_size;
    rlim_t no_of_file;
    std::chrono::seconds wall_timeout;
};

// Common Process Limits
constexpr ProcessLimits COMMON_LIMITS{
    COMMON_CPU_LIMIT,
    COMMON_MEMORY_LIMIT,
    COMMON_FILE_SIZE_LIMIT,
    COMMON_NO_OF_FILE_LIMIT,
    COMMON_WALL_TIMEOUT_SEC
};

// Compiler specific Limits
constexpr ProcessLimits COMPILE_LIMITS{
    COMPILE_CPU_LIMIT,
    COMPILE_MEMORY_LIMIT,
    COMPILE_FILE_SIZE_LIMIT,
    COMMON_NO_OF_FILE_LIMIT,
    COMPILE_WALL_TIMEOUT_SEC
};


// Compile Status
struct Compile_Status {
    int status;
    std::string binary;
    std::string binary_path;
};



// Helper to set limits
static void set_limits(const ProcessLimits& limits) {
    struct rlimit lim{};

    lim.rlim_cur = lim.rlim_max = limits.cpu;
    setrlimit(RLIMIT_CPU, &lim);

    lim.rlim_cur = lim.rlim_max = limits.memory;
    setrlimit(RLIMIT_AS, &lim);

    lim.rlim_cur = lim.rlim_max = limits.file_size;
    setrlimit(RLIMIT_FSIZE, &lim);

    lim.rlim_cur = lim.rlim_max = limits.no_of_file;
    setrlimit(RLIMIT_NOFILE, &lim);

    // Don't generate core dumps.
    lim.rlim_cur = lim.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &lim);
}


//fucntion to start a new process
int new_process(
    const char* path,
    char *args[],
    const int input_fd,
    const int output_fd,
    const int error_fd,
    const ProcessLimits& limits = COMMON_LIMITS
);


//function to compile
Compile_Status compile(int cfd, const char *code);


//delete a file
inline int rm(const std::string& path) {
    return unlink(path.c_str()); 
}


//file compare
int diff(const std::string& file1_path, const std::string& file2_path);


//copy file
int copy_file(const std::string& source_file_path, const std::string& dest_dir);

#endif