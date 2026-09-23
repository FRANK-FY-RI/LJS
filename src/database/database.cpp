#include "database.hpp"

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
