#ifndef __PROCESS_UTILS_HPP
#define __PROCESS_UTILS_HPP

#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <utility>
#include <string>



#define AC 0
#define WA 1
#define RUNTIME_ERROR 2
#define TLE 3
#define MLE 4
#define CHILD_PROCESS_ERROR 5
#define PROCESS_ERROR 6


inline std::string exitsig;




//fucntion to start a new process
int new_process(const char* path, char *args[], const int input_fd, const int output_fd, const int error_fd);


//function to compile
std::pair<int, std::string> compile(int cfd, const char *code);


//delete a file
inline int rm(const std::string& path) {
    return unlink(path.c_str()); 
}


#endif