#pragma once
#include "../libs/crow_all.h"
#include "../database/Database.h"
#include "../patterns/UserFactory.h"  
#include "AuthHandler.h"
#include <string>


inline bool requireAdmin(const SessionData& s) //SessionData = stores logged-in user info
{
    return s.userId != 0 && s.role == "admin";
}

inline void registerAdminRoutes(crow::SimpleApp& app) //app= main server object
{

    CROW_ROUTE(app, "/api/admin/stats").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireAdmin(s)) return crow::response(403, "{\"error\":\"Admin access required\"}");

        Database* db = Database::getInstance(); //get singleton database instance
        
        DBRow books      = db->queryOne("SELECT COUNT(*) AS cnt FROM books");
        DBRow users      = db->queryOne("SELECT COUNT(*) AS cnt FROM users");
        DBRow students   = db->queryOne("SELECT COUNT(*) AS cnt FROM users WHERE role='student'");
        DBRow librarians = db->queryOne("SELECT COUNT(*) AS cnt FROM users WHERE role='librarian'");
        DBRow active     = db->queryOne("SELECT COUNT(*) AS cnt FROM transactions WHERE status='active'");
        DBRow overdue    = db->queryOne("SELECT COUNT(*) AS cnt FROM transactions WHERE status='overdue'");
        DBRow fines      = db->queryOne("SELECT COALESCE(SUM(fine_balance),0) AS total FROM users");
        DBRow seats      = db->queryOne("SELECT COUNT(*) AS cnt FROM seat_bookings WHERE status='confirmed'");
        DBRow pendingBills = db->queryOne("SELECT COUNT(*) AS cnt FROM membership_bills WHERE status='pending'");

        //creating JSON object for response
        crow::json::wvalue resp;
        resp["totalBooks"]     = books.empty()     ? 0 : std::stoi(books["cnt"]);
        resp["totalUsers"]     = users.empty()     ? 0 : std::stoi(users["cnt"]);
        resp["totalStudents"]  = students.empty()  ? 0 : std::stoi(students["cnt"]);
        resp["totalLibrarians"]= librarians.empty()? 0 : std::stoi(librarians["cnt"]);
        resp["activeLoans"]    = active.empty()    ? 0 : std::stoi(active["cnt"]);
        resp["overdueLoans"]   = overdue.empty()   ? 0 : std::stoi(overdue["cnt"]);
        resp["totalFines"]     = fines.empty()     ? 0 : std::stod(fines["total"]);
        resp["activeSeats"]    = seats.empty()     ? 0 : std::stoi(seats["cnt"]);
        resp["pendingBills"]   = pendingBills.empty() ? 0 : std::stoi(pendingBills["cnt"]);

        crow::response res(200, resp.dump()); // 200= OK repsonse, convert JSON to string
        res.add_header("Content-Type", "application/json"); //tell browser reponse is JSON
        return res;
    });

    // get all users
    CROW_ROUTE(app, "/api/admin/users").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireAdmin(s)) return crow::response(403, "{\"error\":\"Admin access required\"}");

        Database* db = Database::getInstance();
        // Optional filter by role
        std::string roleFilter = req.url_params.get("role") ? req.url_params.get("role") : "";
        std::string sql = "SELECT id, name, email, role, membership, fine_balance, is_suspended, created_at FROM users";
        if (!roleFilter.empty() && UserFactory::isValidRole(roleFilter)) {
            sql += " WHERE role='" + db->escape(roleFilter) + "'";
        }
        sql += " ORDER BY role, name ASC";

        DBResult rows = db->query(sql);
        std::vector<crow::json::wvalue> list; //list of JSON users
        for (auto& r : rows) {
            crow::json::wvalue item;
            item["id"]          = std::stoi(r["id"]);
            item["name"]        = r["name"];
            item["email"]       = r["email"];
            item["role"]        = r["role"];
            item["membership"]  = r["membership"];
            item["fineBalance"] = std::stod(r["fine_balance"]);
            item["isSuspended"] = r["is_suspended"] == "1";
            item["createdAt"]   = r["created_at"];
            list.push_back(std::move(item));
        }
        crow::json::wvalue resp;
        resp["users"] = std::move(list);
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // create new user
    CROW_ROUTE(app, "/api/admin/users").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireAdmin(s)) return crow::response(403, "{\"error\":\"Admin access required\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        std::string name     = body["name"].s();
        std::string email    = body["email"].s();
        std::string password = body["password"].s();
        std::string role     = body["role"].s();

        if (name.empty() || email.empty() || password.empty())
            return crow::response(400, "{\"error\":\"Name, email, password required\"}");

       
        if (!UserFactory::isValidRole(role))
            return crow::response(400, "{\"error\":\"Invalid role. Use: student, librarian, admin\"}");

        Database* db = Database::getInstance();
        if (!db->queryOne("SELECT id FROM users WHERE email='" + db->escape(email) + "'").empty())
            return crow::response(409, "{\"error\":\"Email already exists\"}");

        bool ok = db->execute(
            "INSERT INTO users (name, email, password, role) VALUES ('"
            + db->escape(name) + "','" + db->escape(email) + "','"
            + db->escape(password) + "','" + db->escape(role) + "')"
        );
        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["id"]      = ok ? db->lastInsertId() : 0;
        resp["message"] = ok ? "User created successfully." : "Creation failed.";
        return crow::response(ok ? 201 : 500, resp.dump());
    });

    // update user
    CROW_ROUTE(app, "/api/admin/users/<int>").methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, int id) {
        SessionData s = getSession(req);
        if (!requireAdmin(s)) return crow::response(403, "{\"error\":\"Admin access required\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        Database* db = Database::getInstance();
        
        std::string sql = "UPDATE users SET ";
        bool first = true; // used to handle commas correctly in SQL

        if (body.has("role")) {
            std::string role = body["role"].s();
            if (!UserFactory::isValidRole(role))
                return crow::response(400, "{\"error\":\"Invalid role\"}");
            if (!first) sql += ","; sql += "role='" + db->escape(role) + "'"; first = false;
        }
        if (body.has("membership")) {
            if (!first) sql += ",";
            sql += "membership='" + db->escape(body["membership"].s()) + "'"; first = false;
        }
        if (body.has("isSuspended")) {
            if (!first) sql += ",";
            sql += "is_suspended=" + std::string(body["isSuspended"].b() ? "1" : "0"); first = false;
        }
        if (body.has("fineBalance")) {
            if (!first) sql += ",";
            sql += "fine_balance=" + std::to_string(body["fineBalance"].d()); first = false;
        }
        if (first) return crow::response(400, "{\"error\":\"Nothing to update\"}");
        sql += " WHERE id=" + std::to_string(id);

        bool ok = db->execute(sql);
        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["message"] = ok ? "User updated." : "Update failed.";
        return crow::response(ok ? 200 : 500, resp.dump());
    });

    // delete a user account
    CROW_ROUTE(app, "/api/admin/users/<int>").methods(crow::HTTPMethod::Delete)
    ([](const crow::request& req, int id) {
        SessionData s = getSession(req);
        if (!requireAdmin(s)) return crow::response(403, "{\"error\":\"Admin access required\"}");
       
        // Prevent admin from deleting themselves
        if (s.userId == id) return crow::response(400, "{\"error\":\"Cannot delete your own account\"}");

        Database* db = Database::getInstance();
        bool ok = db->execute("DELETE FROM users WHERE id=" + std::to_string(id));
        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["message"] = ok ? "User deleted." : "Delete failed.";
        return crow::response(ok ? 200 : 500, resp.dump());
    });

    // get book requests
    CROW_ROUTE(app, "/api/admin/requests").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireAdmin(s) && s.role != "librarian")
            return crow::response(403, "{\"error\":\"Access denied\"}");

        Database* db = Database::getInstance();
        DBResult rows = db->query(
            "SELECT r.id, r.book_title, r.author, r.reason, r.status, r.admin_note, r.requested_at,"
            " u.name AS user_name FROM book_requests r JOIN users u ON r.user_id=u.id "
            "ORDER BY r.requested_at DESC"
        );
        std::vector<crow::json::wvalue> list;
        for (auto& r : rows) {
            crow::json::wvalue item;
            item["id"]          = std::stoi(r["id"]);
            item["bookTitle"]   = r["book_title"];
            item["author"]      = r["author"];
            item["reason"]      = r["reason"];
            item["status"]      = r["status"];
            item["adminNote"]   = r["admin_note"];
            item["requestedAt"] = r["requested_at"];
            item["userName"]    = r["user_name"];
            list.push_back(std::move(item));
        }
        crow::json::wvalue resp;
        resp["requests"] = std::move(list);
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // approve or reject book request
    CROW_ROUTE(app, "/api/admin/requests/<int>").methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, int id) {
        SessionData s = getSession(req);
        if (!requireAdmin(s)) return crow::response(403, "{\"error\":\"Admin access required\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        std::string status = body["status"].s(); // "approved" or "rejected"
        std::string note   = body["adminNote"].s();

        Database* db = Database::getInstance();
        bool ok = db->execute(
            "UPDATE book_requests SET status='" + db->escape(status)
            + "', admin_note='" + db->escape(note)
            + "' WHERE id=" + std::to_string(id)
        );
        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["message"] = ok ? "Request " + status + "." : "Update failed.";
        return crow::response(ok ? 200 : 500, resp.dump());
    });
}
