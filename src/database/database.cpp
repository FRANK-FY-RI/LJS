#include "database.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <fstream>

//Constructor
Database::Database(const std::string& database_name) {
    if(sqlite3_open(database_name.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error(
            "Unable to connect to database " + 
            database_name + ": " + 
            static_cast<std::string>(sqlite3_errmsg(db))
        );
    } 
}

//Destructor
Database::~Database() {
    sqlite3_close(Database::db); 
}

//Get Handle
sqlite3* Database::handle() const {
    return Database::db;
}

int Database::schema_init(const std::string& file_path) {
    std::ifstream file(file_path); 
    if(!file) return 1;
    std::stringstream buffer;
    buffer << file.rdbuf();
    if(sqlite3_exec(db, buffer.str().c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) return 1;
    return 0;
}
