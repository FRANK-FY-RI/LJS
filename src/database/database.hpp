#ifndef __DATABASE__HPP
#define __DATABASE__HPP


#include "../../third_party/sqlite3/sqlite3.h"
#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>

class Table {
    sqlite3 *db;
    std::string table_name;
    int column_size = 0;
public:

    //Constructor creates a new database or connects to an existing one if already exists
    Table(const std::string& database_name, const std::string& tname); 

    //For simplicity, delete copy and move constructors
    Table(const Table&)=delete;
    Table(Table&&)=delete;

    //Also delete copy and move assignment operators for now
    Table& operator=(const Table&)=delete;
    Table& operator=(Table&&)=delete;

    //Destructor
    ~Table(); 

    void display_table() const; 

    int insert_row(const std::vector<std::string>& row); //insert_row returns 0 on success and 1 on any kind of faliure

};

#endif
