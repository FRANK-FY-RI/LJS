#include "../include/process_utils.hpp"


//function to start a new process
int new_process(
    const char* path,
    char *args[],
    const int input_fd,
    const int output_fd,
    const int error_fd,
    const ProcessLimits& limits
) {
    int status;

    pid_t pid = fork();

    if(pid == -1) {
        return CHILD_PROCESS_ERROR;
    }

    if(pid == 0) {

        // Put child in its own process group so the parent
        // can kill the whole process tree on timeout.
        setpgid(0, 0);

        // Apply resource limits before exec().
        set_limits(limits);

        // redirect input
        if(input_fd != -1) {
            dup2(input_fd, 0);
            close(input_fd);
        }

        // redirect output
        if(output_fd != -1) {
            dup2(output_fd, 1);
            close(output_fd);
        }

        // redirect error
        if(error_fd != -1) {
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);
        }

        execv(path, args);
        _exit(CHILD_PROCESS_ERROR);
    }

    /*
    This line prevents data race for creating a new process group id
    for the child process, otherwise the parent may also get killed 
    due to timeout 
    */
    setpgid(pid, pid);

    const auto start = std::chrono::steady_clock::now();

    while(waitpid(pid, &status, WNOHANG) == 0) {

        if(std::chrono::steady_clock::now() - start > limits.wall_timeout) {

            kill(-pid, SIGKILL);
            waitpid(pid, &status, 0);

            return TLE;
        } 

        std::this_thread::sleep_for(std::chrono::microseconds(100));
    } 

    if(WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return CHILD_PROCESS_ERROR;
}



//Compile function
Compile_Status compile(int cfd, const char *code) {
    Compile_Status status;
    status.binary = static_cast<std::string>("sol") + std::to_string(cfd);
    status.binary_path = temp_dir + status.binary;
    std::vector<char*> compile_args = {
        (char*)"g++",
        const_cast<char*>(code),
        (char*)("-o"),
        const_cast<char*>(status.binary_path.c_str()),
    };
    for(const auto arg:compiler_args) {
        compile_args.push_back(arg);
    }
    compile_args.push_back(NULL);
    status.status = new_process(
        "/usr/bin/g++",
        compile_args.data(),
        -1, -1, cfd,
        COMPILE_LIMITS
    ); 
    return status;
}


//Compare files
int diff(const std::string& file1_path, const std::string& file2_path) {
    std::ifstream file1(file1_path), file2(file2_path);
    if(!file1 || !file2) return PROCESS_ERROR;
    std::string s1, s2, temp;
    auto rtrim = [](std::string& s) {
        while(!s.empty() && !std::isgraph(s.back())) s.pop_back();
    };
    while(true) {
        bool ch1 = static_cast<bool>(std::getline(file1, s1));
        bool ch2 = static_cast<bool>(std::getline(file2, s2));
        if(ch1 != ch2) return 1;
        else if(!ch1) return 0;

        rtrim(s1);
        rtrim(s2);
        if(s1 != s2) return 1;
    } 
    return 0;
}


//copy file
int copy_file(const std::string& source_file_path, const std::string& dest_dir) {
    namespace fs = std::filesystem;
    try {
        fs::copy_file(
            source_file_path,
            fs::path(dest_dir) / fs::path(source_file_path).filename(),
            fs::copy_options::overwrite_existing
        );
    }
    catch (const fs::filesystem_error& e) {
        return PROCESS_ERROR;
    }
    return 0;
}