#ifndef __TABLE__HPP
#define __TABLE__HPP


#include "database.hpp"
#include <string>
#include <vector>

class Table {
    const Database& db;
    const std::string table_name;
    int column_size = 0;
public:

    //Constructor creates a new database or connects to an existing one if already exists
    Table(const Database& database_name, const std::string& tname); 

    //For simplicity, delete copy and move constructors
    Table(const Table&)=delete;
    Table(Table&&)=delete;

    //Also delete copy and move assignment operators for now
    Table& operator=(const Table&)=delete;
    Table& operator=(Table&&)=delete;

    //Destructor
    ~Table()=default; 

    void display_table() const; 

    int insert_row(const std::vector<std::string>& row); //insert_row returns 0 on success and 1 on any kind of faliure

};

#endif
