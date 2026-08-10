#ifndef __CONFIG
#define __CONFIG


#include <vector>
#include <string>

inline const std::string Time_Limit = "2"; // in seconds
inline const std::string Memory_Limit = "1024"; // in KB
const std::vector<char*> compiler_args = {
    (char*)"-std=c++20",
    (char*)"-Wall",
    (char*)"-static-libstdc++"
};

#endif