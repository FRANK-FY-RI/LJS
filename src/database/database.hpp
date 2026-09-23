#ifndef __DATABASE_HPP
#define __DATABASE_HPP


#include "../../third_party/sqlite3/sqlite3.h"
#include <string>

class Database{
    sqlite3 *db = nullptr;
public:
    //Constructor creates a new database or connects to an existing one if already exists
    Database(const std::string& database_name); 

    //For simplicity, delete copy and move constructors
    Database(const Database&)=delete;
    Database(Database&&)=delete;

    //Also delete copy and move assignment operators for now
    Database& operator=(const Database&)=delete;
    Database& operator=(Database&&)=delete;

    //Destructor
    ~Database(); 

    //Get Handle
    sqlite3* handle() const;

    /*
    Initialize Schema
    Takes schema file(.sql) path and initializes the schema
    Returns 0 on success and 1 otherwise
    */
    int schema_init(const std::string& file_path);
};

#endif
