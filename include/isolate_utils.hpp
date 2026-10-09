#ifndef __ISOLATE_UTILS_HPP
#define __ISOLATE_UTILS_HPP

#include <string>
#include "threadsafe_queue.hpp"
#include "verdict.hpp"




class Isolate {
    bool is_active;

    static threadsafe_queue<int> free_box_ids;
    static std::once_flag free_box_ids_init_flag;
    static void init_free_box_ids();

    //Isolate Cleaner
    Verdict isolate_cleanup();

public: 
    std::string box_id;
    std::string box_path;

    //Constructor
    Isolate() : is_active(false) {}

    //Delete Copy Constructor and Copy Assignment operator
    Isolate(const Isolate&)=delete;
    Isolate& operator=(const Isolate&)=delete;

    //Delete Move Constructor and Move Assignment operator
    Isolate(Isolate&&)=delete;
    Isolate& operator=(Isolate&&)=delete;

    //Destructor
    ~Isolate();

    //Isolate sandbox Initialization
    Verdict isolate_init();


    //Isolate Runner
    Verdict isolate_run(
            const std::string& binary_file,
            const std::string& input_file
            );




    //Metadata Parser
    Verdict metadata_verdict(const std::string& metadata_file_path);

};





#endif
