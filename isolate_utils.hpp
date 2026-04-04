#ifndef __ISOLATE_UTILS_HPP
#define __ISOLATE_UTILS_HPP

#include "process_utils.hpp"
#include <fcntl.h>
#include <utility>
#include <string>
#include <fstream>
#include <sstream>


//Isolate Runner
std::pair<int, std::string> isolate_run(const std::string& binary_file, const std::string& binary_file_path, const std::string& input_file, const std::string& input_file_path);


//Isolate Cleaner
int isolate_cleanup(std::string boxid);


//Metadata Parser
int metadata_verdict(const std::string& metadata_file_path);




#endif