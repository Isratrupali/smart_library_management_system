// DESIGN PATTERN: SINGLETON


#pragma once                   

#include "../libs/sqlite3.h"   
#include <string>              
#include <vector>              
#include <map>                 
#include <stdexcept>            
#include <iostream>           
#include <fstream>            
#include <sstream>         

using DBRow = std::map<std::string, std::string>;  // One row: column name; cell value
using DBResult = std::vector<DBRow>;               // Multiple rows: a list of DBRow maps

// CLASS: Database; The Singleton database wrapper
class Database {
private:
    static Database* instance; // holds one object; singleton

    sqlite3* db; // databse connection

    Database() : db(nullptr) //constructor is private
    {
        

        int result = sqlite3_open("slms.db", &db); // opens or creates database file

        //if opening fails
        if (result != SQLITE_OK) {
           
            std::string err = sqlite3_errmsg(db);
            sqlite3_close(db);                     // Close the failed connection
            db = nullptr;                          // Set pointer to null so we know it failed
           
            throw std::runtime_error("[Database] Failed to open slms.db: " + err);
        }


        execute("PRAGMA journal_mode=WAL;"); //WAL mode = faster and supports multiple users

        execute("PRAGMA foreign_keys=ON;"); // enables foreign key constraints

        std::cout << "[Database] SQLite connection opened successfully (slms.db)" << std::endl; //log msg
    }

    // [SINGLETON] Private destructor; only the class can destroy itself.
    // Called when Database::destroy() is invoked (at server shutdown).
    ~Database() {
        if (db) {                    // Only close if the connection is actually open
            sqlite3_close(db);       // close the databse safely
            db = nullptr;            // Reset pointer to null for safety
            std::cout << "[Database] SQLite connection closed." << std::endl;
        }
    }

    //prevents copy
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete; // Also delete copy assignment

public:
    // getInstance: THE Singleton access point.
   
    static Database* getInstance() {
        if (!instance) {                    
            instance = new Database();      
        }
        return instance;                   
    }

    //runs SQL qurey
    bool execute(const std::string& sql) {
        char* errMsg = nullptr; // SQLite writes error message here if query fails

        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg); //executes sql

        if (rc != SQLITE_OK) {                          //if failed: print error, free memory, return false
            std::cerr << "[Database] SQL Error: " << (errMsg ? errMsg : "unknown") << std::endl;
            std::cerr << "[Database] Failed SQL: " << sql.substr(0, 200) << std::endl;
            sqlite3_free(errMsg);                       
            return false;                               
        }
        return true; // Query ran successfully
    }

    
   
    // Each DBRow maps column names to values
   
    DBResult query(const std::string& sql) //used for select quires
    {
        DBResult results;      
        sqlite3_stmt* stmt;    


        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr); //compile sql

        if (rc != SQLITE_OK) {  // Did compilation fail?
            std::cerr << "[Database] Query prepare error: " << sqlite3_errmsg(db) << std::endl;
            std::cerr << "[Database] SQL was: " << sql.substr(0, 200) << std::endl;
            return results;     // Return empty results on error
        }

        // Get the number of columns the SELECT statement returns
        int colCount = sqlite3_column_count(stmt); // e.g. 5 if SELECT has 5 columns

        // for each row
        while (sqlite3_step(stmt) == SQLITE_ROW) { // SQLITE_ROW means we got a data row
            DBRow row; // One row as a map: column_name, value

            // Loop through each column in this row
            for (int i = 0; i < colCount; i++) {
                
                std::string colName = sqlite3_column_name(stmt, i);

                const char* val = (const char*)sqlite3_column_text(stmt, i);

                // Store in the row map
                row[colName] = val ? val : ""; // If NULL, store "" (empty string)
            }

            results.push_back(row); // Add this row to our results list
        }

        sqlite3_finalize(stmt);
        return results;         // Return all the rows we collected
    }

    //Run a SELECT and return only the FIRST row.

    DBRow queryOne(const std::string& sql) {
        DBResult rows = query(sql);         // Run the full query
        if (rows.empty()) return DBRow();   // No rows found; return empty map
        return rows[0];                     // Return just the first row
    }

    // Get the ID of the last inserted row.
    
    int lastInsertId() {
       
        return (int)sqlite3_last_insert_rowid(db);
    }

    
    
    std::string escape(const std::string& s) {
        std::string result;          // Will hold the escaped version
        result.reserve(s.size());    // Pre-allocate memory (optimization)

        for (char c : s) {           // Loop through every character in the input
            if (c == '\'') {         // Found a single quote?
                result += "''";      // Replace it with TWO single quotes (SQL escape)
            } else {
                result += c;         // Normal character; copy it unchanged
            }
        }
        return result;               // Return the safe, escaped string
    }

 
    // runSqlFile: Read a .sql file and execute all statements.
    
    bool runSqlFile(const std::string& filepath) {
        std::ifstream file(filepath);           // Open the SQL file for reading
        if (!file.is_open()) {                  // Could not find or open the file?
            std::cerr << "[Database] Cannot open SQL file: " << filepath << std::endl;
            return false;                       // Report failure
        }

        // Read file line by line and strip comments
        std::string content;
        std::string line;
        while (std::getline(file, line)) {
            size_t commentPos = line.find("--");
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos); // Remove the comment part
            }
            content += line + " "; // Add line to content with a space
        }

        // Split the file on semicolons; each statement ends with ';'
        std::string stmt;
        for (size_t i = 0; i < content.size(); i++) {
            char c = content[i];         // Current character
            stmt += c;                   // Add it to the current statement

            if (c == ';') {              // Found end of statement?
                // Trim whitespace from the statement before executing
                std::string trimmed = stmt;
                while (!trimmed.empty() && (trimmed[0]==' '||trimmed[0]=='\n'||trimmed[0]=='\r'||trimmed[0]=='\t'))
                    trimmed.erase(0, 1); // Remove leading whitespace characters

                if (!trimmed.empty() && trimmed != ";") { // Skip empty statements
                    execute(trimmed);     // Run this SQL statement
                }
                stmt.clear();            // Reset for the next statement
            }
        }

        std::cout << "[Database] Executed SQL file: " << filepath << std::endl;
        return true;
    }

  
    // destroy: Cleanup; call this when the server shuts down.
    // Deletes the singleton instance and closes the DB connection.

    static void destroy() {
        if (instance) {         // Is there an instance to destroy?
            delete instance;    // Calls the destructor (closes DB connection)
            instance = nullptr; // Reset to null so getInstance() creates fresh next time
        }
    }
};


Database* Database::instance = nullptr; // Initialize the Singleton pointer to null
