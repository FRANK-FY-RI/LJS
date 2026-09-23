#include "database.hpp"

//Constructor
Table::Table(const std::string& database_name, const std::string& tname) : table_name(tname) {
    if(sqlite3_open(database_name.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error(
                "Unable to connect to database " + 
                database_name + ": " + 
                static_cast<std::string>(sqlite3_errmsg(db))
                );
    }
    sqlite3_stmt *sql = nullptr;
    const std::string stmt = "PRAGMA table_info(" + table_name + ")";
    if(sqlite3_prepare_v2(db, stmt.c_str(), -1, &sql, nullptr) != SQLITE_OK) {
        const std::string errmsg = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw std::runtime_error(
                "Unable to inspect table " + 
                table_name + ": " + errmsg
        );
    }
    while(sqlite3_step(sql) == SQLITE_ROW) {
        column_size++;
    }
    sqlite3_finalize(sql);
}

//Destructor
Table::~Table() {
    sqlite3_close(db);
}

void Table::display_table() const {
    const std::string stmt = "SELECT * FROM " + table_name;
    sqlite3_stmt *sql = nullptr;
    if(sqlite3_prepare_v2(db, stmt.c_str(), -1, &sql, nullptr) != SQLITE_OK) {
        const std::string errmsg = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw std::runtime_error(
                "Unable to prepare SELECT statement on table " + 
                table_name + ": " + errmsg
        );
    }
    int records = 0;
    while(sqlite3_step(sql) == SQLITE_ROW) {
        records++;
        for(int i = 0; i<column_size; i++) {
            if(i) std::cout<<" | ";
            std::cout<<sqlite3_column_text(sql, i);
        }
        std::cout<<'\n';
    }
    sqlite3_finalize(sql);
    std::cout<<"Total No.of records: "<<records<<'\n';
}

int Table::insert_row(const std::vector<std::string>& row) {
    if(row.size() != column_size) return 1;
    std::string stmt = "INSERT INTO " + table_name + " VALUES (";
    for(int i = 0; i<row.size(); i++) {
        if(i) stmt += ", ";
        stmt += "?";
    }
    stmt += ")";

    //prepare INSERT
    sqlite3_stmt *sql = nullptr;
    if(sqlite3_prepare_v2(
                db, 
                stmt.c_str(), 
                -1, 
                &sql, 
                nullptr
                ) != SQLITE_OK)
    {
        return 1; 
    }

    //Bind all values
    for(int i = 0; i<row.size(); i++) {
        if(sqlite3_bind_text(
                    sql,
                    i+1,
                    row[i].c_str(),
                    -1,
                    SQLITE_TRANSIENT
                    ) != SQLITE_OK)
        {
            sqlite3_finalize(sql);
            return 1; 
        }
    }

    //Execute
    if(sqlite3_step(sql) != SQLITE_DONE) {
        sqlite3_finalize(sql);
        return 1;
    }
    sqlite3_finalize(sql);
    return 0;
}
