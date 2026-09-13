#ifndef __VERDICT_HPP
#define __VERDICT_HPP

#include <string>

enum class Verdict {
    AC,
    WA,
    COMPILATION_ERROR,
    COMPILATION_TLE,
    TLE,
    MLE,
    RUNTIME_ERROR,
    PROCESS_ERROR,
    CHILD_PROCESS_ERROR,
    CONNECTION_ERROR,
    SUCCESS,
    FAILURE
};

#endif
