#ifndef __ISOLATE_UTILS_HPP
#define __ISOLATE_UTILS_HPP

#include "process_utils.hpp"
#include <string>
#include <atomic>
#include <vector>
#include "config.hpp"


static std::atomic<int> box_cnt{0};


struct Isolate_Init_status{
    int status;
    std::string box_id;
    std::string box_path;
};

//Isolate sandbox Initialization
Isolate_Init_status isolate_init();


//Isolate Runner
int isolate_run(
    const std::string& box_id,
    const std::string& binary_file,
    const std::string& input_file
);


//Isolate Cleaner
int isolate_cleanup(const std::string& boxid);


//Metadata Parser
int metadata_verdict(const std::string& metadata_file_path);




#endif