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


//Verdict to string
inline std::string verdict_to_string(const Verdict& v) {
    switch (v) {
        case Verdict::AC:
            return "AC";
        case Verdict::WA:
            return "WA";
        case Verdict::COMPILATION_ERROR:
            return "COMPILATION_ERROR";
        case Verdict::COMPILATION_TLE:
            return "COMPILATION_TLE";
        case Verdict::TLE:
            return "TLE";
        case Verdict::MLE:
            return "MLE";
        case Verdict::RUNTIME_ERROR:
            return "RUNTIME_ERROR";
        case Verdict::PROCESS_ERROR:
            return "PROCESS_ERROR";
        case Verdict::CHILD_PROCESS_ERROR:
            return "CHILD_PROCESS_ERROR";
        case Verdict::CONNECTION_ERROR:
            return "CONNECTION_ERROR";
        case Verdict::SUCCESS:
            return "SUCCESS";
        case Verdict::FAILURE:
            return "FAILURE";
    }

    return "UNKNOWN";
}

#endif
