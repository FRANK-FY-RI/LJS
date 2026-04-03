#ifndef __ISOLATE_UTILS_HPP
#define __ISOLATE_UTILS_HPP

#include "process_utils.hpp"
#include <fcntl.h>
#include <utility>
#include <string>
#include <fstream>
#include <sstream>

using std::pair;
using std::string;


//Isolate Runner
pair<int, string> isolate_run(const string& binary_file, const string& binary_file_path, const string& input_file, const string& input_file_path);


//Isolate Cleaner
int isolate_cleanup(string boxid);


//Metadata Parser
int metadata_verdict(const string& metadata_file_path);




#endif