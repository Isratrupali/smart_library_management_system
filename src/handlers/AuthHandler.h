// PURPOSE: Handles login, register, and logout API routes.
// Uses the Factory Pattern to validate roles.

#pragma once
#include "../libs/crow_all.h"       
#include "../database/Database.h"    
#include "../patterns/UserFactory.h" 
#include <map>                     
#include <string>                    
#include <ctime>                     
#include <sstream>             

// stores logged-in user information
struct SessionData {
    int         userId   = 0;    
    std::string role     = "";    
    std::string name     = "";    
    std::string email    = "";    
};

// token - SessionData mapping
std::map<std::string, SessionData> sessions; // In-memory session store

// create session token
inline std::string makeToken(int userId) {
    // Combine current time and user ID to make a unique-ish token string
    std::ostringstream ss;
    ss << "tok_" << time(nullptr) << "_" << userId; 
    return ss.str();
}


// get session from cookie
inline SessionData getSession(const crow::request& req) {
    // Read the "Cookie" header from the HTTP request
    std::string cookieHeader = req.get_header_value("Cookie");
    if (cookieHeader.empty()) return {}; // No cookie - not logged in

    // Find the "slms_token=" cookie in the header string
    std::string key = "slms_token=";
    size_t pos = cookieHeader.find(key); // find our session token
    if (pos == std::string::npos) return {}; // Token not found

    
    // Extract the token value (from after "slms_token=" to next ";" or end)
    pos += key.size(); // Move past the "slms_token=" prefix
    size_t end = cookieHeader.find(';', pos); // Find next semicolon
    std::string token = (end == std::string::npos)
        ? cookieHeader.substr(pos)
        : cookieHeader.substr(pos, end - pos); // Take until the semicolon

  
        // extract token value
    auto it = sessions.find(token); 
    if (it == sessions.end()) return {}; // Not found - invalid/expired token
    return it->second; 
}

// Convert user data to JSON
inline crow::json::wvalue userToJson(const SessionData& s) {
    crow::json::wvalue j;  
    j["userId"] = s.userId; 
    j["name"]   = s.name;  
    j["email"]  = s.email;
    j["role"]   = s.role;
    return j;
}


// Registers all auth-related routes on the Crow app.

inline void registerAuthRoutes(crow::SimpleApp& app) {

    CROW_ROUTE(app, "/api/login").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        // Parse the JSON body sent by the frontend login form
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON body\"}");

        // Extract email and password from the JSON
        std::string email    = body["email"].s();    // .s() gets the string value
        std::string password = body["password"].s();

        
        if (email.empty() || password.empty()) {
            crow::json::wvalue err;
            err["error"] = "Email and password are required.";
            return crow::response(400, err.dump()); // 400 = Bad Request
        }

       
        Database* db = Database::getInstance(); 
        DBRow user = db->queryOne(
            "SELECT id, name, email, role, is_suspended FROM users WHERE email='"
            + db->escape(email) + "' AND password='" + db->escape(password) + "'"
        );

        if (user.empty()) {
           
            crow::json::wvalue err;
            err["error"] = "Invalid email or password.";
            return crow::response(401, err.dump());
        }

        // Check if the account has been suspended by admin
        if (user["is_suspended"] == "1") {
            crow::json::wvalue err;
            err["error"] = "Your account is suspended. Contact the library admin.";
            return crow::response(403, err.dump());
        }

        // Create a session token for this user
        int uid = std::stoi(user["id"]);
        std::string token = makeToken(uid); // Generate unique session token

        // Store the session in memory
        sessions[token] = { uid, user["role"], user["name"], user["email"] };

        // Build the success response JSON
        crow::json::wvalue resp;
        resp["success"]  = true;
        resp["token"]    = token;                         
        resp["role"]     = user["role"];                   
        resp["name"]     = user["name"];                
        resp["userId"]   = uid;                           
        resp["dashboard"]= UserFactory::create(user["role"])->getDashboardPath(); 


        crow::response res(200, resp.dump());              // 200 = OK
        res.add_header("Content-Type", "application/json");
        
        res.add_header("Set-Cookie", "slms_token=" + token + "; Path=/; HttpOnly");
        return res;
    });

    // Register

    CROW_ROUTE(app, "/api/register").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        std::string name     = body["name"].s();
        std::string email    = body["email"].s();
        std::string password = body["password"].s();

        
        if (name.empty() || email.empty() || password.empty()) {
            crow::json::wvalue err; err["error"] = "All fields are required.";
            return crow::response(400, err.dump());
        }
        
        
        if (password.size() < 6) {
            crow::json::wvalue err; err["error"] = "Password must be at least 6 characters.";
            return crow::response(400, err.dump());
        }

        Database* db = Database::getInstance();

        // Check if email is already registered
        DBRow existing = db->queryOne(
            "SELECT id FROM users WHERE email='" + db->escape(email) + "'"
        );
        if (!existing.empty()) {
            crow::json::wvalue err; err["error"] = "Email already registered.";
            return crow::response(409, err.dump()); 
        }

        // Insert the new student account
        bool ok = db->execute(
            "INSERT INTO users (name, email, password, role, membership) VALUES ('"
            + db->escape(name) + "', '"
            + db->escape(email) + "', '"
            + db->escape(password) + "', 'student', 'basic')"
        );
        if (!ok) {
            crow::json::wvalue err; err["error"] = "Registration failed. Try again.";
            return crow::response(500, err.dump());
        }

        crow::json::wvalue resp;
        resp["success"] = true;
        resp["message"] = "Account created successfully! Please log in.";
        return crow::response(201, resp.dump()); // 201 = Created
    });

    // Log out
    CROW_ROUTE(app, "/api/logout").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        // Read the session token from the cookie
        std::string cookieHeader = req.get_header_value("Cookie");
        std::string key = "slms_token=";
        size_t pos = cookieHeader.find(key);
        if (pos != std::string::npos) {
            pos += key.size();
            size_t end = cookieHeader.find(';', pos);
            std::string token = (end == std::string::npos)
                ? cookieHeader.substr(pos)
                : cookieHeader.substr(pos, end - pos);
            sessions.erase(token); // Remove the session from memory
        }

        // Clear the cookie by setting Max-Age=0 (tells browser to delete it)
        crow::response res(200, "{\"success\":true}");
        res.add_header("Content-Type", "application/json");
        res.add_header("Set-Cookie", "slms_token=; Path=/; Max-Age=0");
        return res;
    });

    // return into the current log in user
    CROW_ROUTE(app, "/api/me").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData session = getSession(req); // Get session from cookie
        if (session.userId == 0) {
            crow::json::wvalue err; err["error"] = "Not logged in.";
            return crow::response(401, err.dump());
        }
        // Return the current user's session info as JSON
        Database* db = Database::getInstance();
        DBRow user = db->queryOne(
            "SELECT id, name, email, role, membership, fine_balance FROM users WHERE id="
            + std::to_string(session.userId)
        );
        crow::json::wvalue resp;
        resp["userId"]     = std::stoi(user["id"]);
        resp["name"]       = user["name"];
        resp["email"]      = user["email"];
        resp["role"]       = user["role"];
        resp["membership"] = user["membership"];
        resp["fineBalance"]= std::stod(user["fine_balance"]);
        resp["loggedIn"]   = true;
        return crow::response(200, resp.dump());
    });
}
