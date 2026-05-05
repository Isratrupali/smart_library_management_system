// PURPOSE: All librarian-specific API routes.

#pragma once
#include "../libs/crow_all.h"
#include "../database/Database.h"
#include "../patterns/Facade.h"  
#include "AuthHandler.h"
#include <string>
#include <vector>

// requires librarian OR admin role
inline bool requireLibrarian(const SessionData& s) {
    return s.userId != 0 && (s.role == "librarian" || s.role == "admin");
}

// This function registers all librarian API routes
inline void registerLibrarianRoutes(crow::SimpleApp& app) {

    CROW_ROUTE(app, "/api/librarian/stats").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        Database* db = Database::getInstance();

        DBRow totalBooks    = db->queryOne("SELECT COUNT(*) AS cnt FROM books");
        DBRow totalStudents = db->queryOne("SELECT COUNT(*) AS cnt FROM users WHERE role='student'");
        DBRow activeLoans   = db->queryOne("SELECT COUNT(*) AS cnt FROM transactions WHERE status='active'");
        DBRow overdueLoans  = db->queryOne("SELECT COUNT(*) AS cnt FROM transactions WHERE status='overdue'");
        DBRow totalFines    = db->queryOne("SELECT COALESCE(SUM(fine_balance),0) AS total FROM users");
        DBRow unreadRequests= db->queryOne("SELECT COUNT(*) AS cnt FROM book_requests WHERE status='pending'");
        DBRow pendingBills  = db->queryOne("SELECT COUNT(*) AS cnt FROM membership_bills WHERE status='pending'");

        crow::json::wvalue resp;
        resp["totalBooks"]     = totalBooks.empty()     ? 0 : std::stoi(totalBooks["cnt"]);
        resp["totalStudents"]  = totalStudents.empty()  ? 0 : std::stoi(totalStudents["cnt"]);
        resp["activeLoans"]    = activeLoans.empty()    ? 0 : std::stoi(activeLoans["cnt"]);
        resp["overdueLoans"]   = overdueLoans.empty()   ? 0 : std::stoi(overdueLoans["cnt"]);
        resp["totalFines"]     = totalFines.empty()     ? 0 : std::stod(totalFines["total"]);
        resp["pendingRequests"]= unreadRequests.empty() ? 0 : std::stoi(unreadRequests["cnt"]);
        resp["pendingBills"]   = pendingBills.empty()   ? 0 : std::stoi(pendingBills["cnt"]);

        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

   
    // All active borrowing records

    CROW_ROUTE(app, "/api/librarian/borrows").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        Database* db = Database::getInstance();
       
        //get user info, book info, borrow info
        DBResult rows = db->query(
            "SELECT t.id, t.borrow_date, t.due_date, t.fine, t.status,"
            " u.name AS user_name, u.email, b.title, b.author "
            "FROM transactions t "
            "JOIN users u ON t.user_id=u.id "
            "JOIN books b ON t.book_id=b.id "
            "WHERE t.status IN ('active','overdue') "
            "ORDER BY t.due_date ASC" 
        );

        std::vector<crow::json::wvalue> list;
        for (auto& r : rows) //loop through all borrow record
        {
            crow::json::wvalue item;
            //coverting DB rows into JSON object
            item["id"]         = std::stoi(r["id"]);
            item["userName"]   = r["user_name"];
            item["email"]      = r["email"];
            item["title"]      = r["title"];
            item["author"]     = r["author"];
            item["borrowDate"] = r["borrow_date"];
            item["dueDate"]    = r["due_date"];
            item["fine"]       = std::stod(r["fine"]);
            item["status"]     = r["status"];
            list.push_back(std::move(item));
        }
        crow::json::wvalue resp;
        resp["borrows"] = std::move(list); //sent list of all borrows
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    
    // shows only overdue books
    CROW_ROUTE(app, "/api/librarian/overdue").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        Database* db = Database::getInstance();
        DBResult rows = db->query(
            "SELECT t.id, t.due_date, t.fine, u.name, u.email, b.title "
            "FROM transactions t "
            "JOIN users u ON t.user_id=u.id "
            "JOIN books b ON t.book_id=b.id "
            "WHERE t.status='overdue' ORDER BY t.due_date ASC" //SQL filter: only for overdue
        );

        std::vector<crow::json::wvalue> list;
        for (auto& r : rows) {
            crow::json::wvalue item;
            item["id"]      = std::stoi(r["id"]);
            item["name"]    = r["name"];
            item["email"]   = r["email"];
            item["title"]   = r["title"];
            item["dueDate"] = r["due_date"];
            item["fine"]    = std::stod(r["fine"]);
            list.push_back(std::move(item));
        }
        crow::json::wvalue resp;
        resp["overdue"] = std::move(list);
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // Get all student
    CROW_ROUTE(app, "/api/librarian/students").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        Database* db = Database::getInstance();
        DBResult rows = db->query(
            "SELECT id, name, email, membership, fine_balance, is_suspended, created_at "
            "FROM users WHERE role='student' ORDER BY name ASC"
        );

        std::vector<crow::json::wvalue> list;
        for (auto& r : rows) {
            crow::json::wvalue item;
            item["id"]          = std::stoi(r["id"]);
            item["name"]        = r["name"];
            item["email"]       = r["email"];
            item["membership"]  = r["membership"];
            item["fineBalance"] = std::stod(r["fine_balance"]);
            item["isSuspended"] = r["is_suspended"] == "1";
            item["createdAt"]   = r["created_at"];
            list.push_back(std::move(item));
        }
        crow::json::wvalue resp;
        resp["students"] = std::move(list);
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // return book - Facade Design Pattern
    //Instead of writing everythinh here, it calls returnBook()
    CROW_ROUTE(app, "/api/librarian/return").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        int txId = body["transactionId"].i();
        Database* db = Database::getInstance();
        LibraryFacade facade(db);             // Reuse the Facade Pattern
        FacadeResult result = facade.returnBook(txId);

        crow::json::wvalue resp;
        resp["success"] = result.success;
        resp["message"] = result.message;
        return crow::response(result.success ? 200 : 400, resp.dump());
    });

    // Send notification to user
    CROW_ROUTE(app, "/api/librarian/notify").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        int         targetUserId = body["userId"].i();
        std::string message      = body["message"].s();

        if (message.empty()) return crow::response(400, "{\"error\":\"Message required\"}");

        Database* db = Database::getInstance();
        bool ok = db->execute(
            "INSERT INTO notifications (user_id, message, is_read) VALUES (" //stores message in database
            + std::to_string(targetUserId) + ", '"
            + db->escape(message) + "', 0)"
        );

        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["message"] = ok ? "Notification sent." : "Failed to send.";
        return crow::response(ok ? 201 : 500, resp.dump());
    });

    // billing system
    
    CROW_ROUTE(app, "/api/librarian/billing").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        std::string st = req.url_params.get("status") ? req.url_params.get("status") : "all";
        Database* db = Database::getInstance();
        std::string sql =
            "SELECT b.id, b.user_id, b.target_tier, b.amount_bdt, b.status, b.created_at, b.paid_at,"
            " b.recorded_by_user_id, b.description, u.name AS user_name, u.email "
            "FROM membership_bills b JOIN users u ON b.user_id=u.id WHERE 1=1";
        if (st == "pending" || st == "paid")
            sql += " AND b.status='" + db->escape(st) + "'";
        sql += " ORDER BY b.status='pending' DESC, b.created_at DESC";

        DBResult rows = db->query(sql);
        std::vector<crow::json::wvalue> list;
        for (auto& r : rows) {
            crow::json::wvalue item;
            item["id"]                 = std::stoi(r["id"]);
            item["userId"]             = std::stoi(r["user_id"]);
            item["userName"]           = r["user_name"];
            item["email"]              = r["email"];
            item["targetTier"]         = r["target_tier"];
            item["amountBdt"]          = std::stod(r["amount_bdt"]);
            item["status"]             = r["status"];
            item["createdAt"]          = r["created_at"];
            item["paidAt"]             = r["paid_at"];
            item["recordedByUserId"]   = r["recorded_by_user_id"].empty() ? 0 : std::stoi(r["recorded_by_user_id"]);
            item["description"]        = r["description"];
            list.push_back(std::move(item));
        }
        crow::json::wvalue resp;
        resp["bills"] = std::move(list);
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // Record payment and Upgrade membership
    CROW_ROUTE(app, "/api/librarian/billing/mark-paid").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        auto body = crow::json::load(req.body);
        if (!body || !body.has("billId"))
            return crow::response(400, "{\"error\":\"billId required\"}");

        int billId = body["billId"].i();
        Database* db = Database::getInstance();
        DBRow bill = db->queryOne(
            "SELECT id, user_id, target_tier, amount_bdt, status FROM membership_bills WHERE id="
            + std::to_string(billId)
        );
        if (bill.empty())
            return crow::response(404, "{\"error\":\"Invoice not found\"}");
        if (bill["status"] != "pending")
            return crow::response(400, "{\"error\":\"Invoice is not pending\"}");

        int userId = std::stoi(bill["user_id"]);
        std::string tier = bill["target_tier"];
        double amt = std::stod(bill["amount_bdt"]);

        bool ok = db->execute(
            "UPDATE membership_bills SET status='paid', paid_at=datetime('now'), recorded_by_user_id="
            + std::to_string(s.userId) + " WHERE id=" + std::to_string(billId)
        );
        if (!ok) return crow::response(500, "{\"error\":\"Update failed\"}");

        db->execute(
            "UPDATE users SET membership='" + db->escape(tier) + "' WHERE id=" + std::to_string(userId)
        );

        std::string msg = "[Billing] Payment received: " + std::to_string((int)amt)
            + " BDT for " + tier + " membership. Your plan is now active. Thank you!";
        db->execute(
            "INSERT INTO notifications (user_id, message, is_read) VALUES ("
            + std::to_string(userId) + ", '" + db->escape(msg) + "', 0)"
        );

        crow::json::wvalue resp;
        resp["success"] = true;
        resp["message"] = "Payment recorded and membership updated.";
        return crow::response(200, resp.dump());
    });

    //  Notify student about unpaid invoice
    
    CROW_ROUTE(app, "/api/librarian/billing/remind").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        auto body = crow::json::load(req.body);
        if (!body || !body.has("billId"))
            return crow::response(400, "{\"error\":\"billId required\"}");

        int billId = body["billId"].i();
        Database* db = Database::getInstance();
        DBRow bill = db->queryOne(
            "SELECT b.user_id, b.target_tier, b.amount_bdt, b.status FROM membership_bills b WHERE b.id="
            + std::to_string(billId)
        );
        if (bill.empty())
            return crow::response(404, "{\"error\":\"Invoice not found\"}");
        if (bill["status"] != "pending")
            return crow::response(400, "{\"error\":\"Invoice is not pending\"}");

        int userId = std::stoi(bill["user_id"]);
        std::string tier = bill["target_tier"];
        double amt = std::stod(bill["amount_bdt"]);

        std::string msg = "[Billing reminder] Invoice #" + std::to_string(billId)
            + ": You still owe " + std::to_string((int)amt) + " BDT for "
            + tier + " membership. Please visit the library to complete payment.";
        bool ok = db->execute(
            "INSERT INTO notifications (user_id, message, is_read) VALUES ("
            + std::to_string(userId) + ", '" + db->escape(msg) + "', 0)"
        );

        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["message"] = ok ? "Reminder sent." : "Failed.";
        return crow::response(ok ? 200 : 500, resp.dump());
    });


    // GET /api/librarian/seats; All confirmed seat bookings for a date
    // Query: ?date=YYYY-MM-DD (optional; default today)

    CROW_ROUTE(app, "/api/librarian/seats").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        std::string date = req.url_params.get("date") ? req.url_params.get("date") : "";
        Database* db = Database::getInstance();
        std::string dateFilter = date.empty() ? "date('now')" : ("'" + db->escape(date) + "'");

        DBResult rows = db->query(
            "SELECT sb.id, sb.user_id, sb.seat_number, sb.room, sb.time_slot, sb.booking_date, sb.status, "
            " u.name AS user_name, u.email "
            "FROM seat_bookings sb JOIN users u ON sb.user_id = u.id "
            "WHERE sb.booking_date=" + dateFilter + " AND sb.status='confirmed' "
            "ORDER BY sb.room, sb.time_slot, sb.seat_number"
        );

        std::vector<crow::json::wvalue> list;
        for (auto& r : rows) {
            crow::json::wvalue item;
            item["id"]          = std::stoi(r["id"]);
            item["userId"]      = std::stoi(r["user_id"]);
            item["userName"]    = r["user_name"];
            item["email"]       = r["email"];
            item["seatNumber"]  = r["seat_number"];
            item["room"]        = r["room"];
            item["timeSlot"]    = r["time_slot"];
            item["bookingDate"] = r["booking_date"];
            item["status"]      = r["status"];
            list.push_back(std::move(item));
        }
        crow::json::wvalue resp;
        resp["bookings"] = std::move(list);
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    //  Reserve a seat for a student
    
    CROW_ROUTE(app, "/api/librarian/seats/book").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (!requireLibrarian(s)) return crow::response(403, "{\"error\":\"Access denied\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        int targetUserId = body["userId"].i();
        std::string seat     = body["seatNumber"].s();
        std::string room     = body["room"].s();
        std::string date     = body["date"].s();
        std::string timeSlot = body["timeSlot"].s();

        if (targetUserId <= 0 || seat.empty() || date.empty() || timeSlot.empty()) {
            return crow::response(400, "{\"error\":\"userId, seat, date, and time slot required\"}");
        }

        Database* db = Database::getInstance();
        DBRow user = db->queryOne(
            "SELECT id, role, name FROM users WHERE id=" + std::to_string(targetUserId)
        );
        if (user.empty() || user["role"] != "student")
            return crow::response(400, "{\"error\":\"Target must be a student account\"}");

        std::string stuName = user["name"];

        DBRow existing = db->queryOne(
            "SELECT id FROM seat_bookings WHERE seat_number='" + db->escape(seat)
            + "' AND room='" + db->escape(room)
            + "' AND booking_date='" + db->escape(date)
            + "' AND time_slot='" + db->escape(timeSlot)
            + "' AND status='confirmed'"
        );
        if (!existing.empty())
            return crow::response(409, "{\"error\":\"This seat is already booked for that time\"}");

        bool ok = db->execute(
            "INSERT INTO seat_bookings (user_id, seat_number, room, booking_date, time_slot) VALUES ("
            + std::to_string(targetUserId) + ",'" + db->escape(seat) + "','"
            + db->escape(room) + "','" + db->escape(date) + "','" + db->escape(timeSlot) + "')"
        );

        if (ok) {
            std::string msg = "A staff member booked seat " + seat + " in " + room + " for you on "
                + date + " (" + timeSlot + ").";
            db->execute(
                "INSERT INTO notifications (user_id, message, is_read) VALUES ("
                + std::to_string(targetUserId) + ", '" + db->escape(msg) + "', 0)"
            );
        }

        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["message"] = ok ? ("Seat " + seat + " booked for " + stuName) : "Booking failed.";
        crow::response out(ok ? 201 : 500, resp.dump());
        out.add_header("Content-Type", "application/json");
        return out;
    });
}
