// PURPOSE: All student-specific API routes.
#pragma once
#include "../libs/crow_all.h"
#include "../database/Database.h"
#include "../patterns/Facade.h"     // LibraryFacade (Borrow/Return)
#include "AuthHandler.h"            // getSession()
#include <string>

namespace {
inline int membershipTierRank(const std::string& t) {
    if (t == "basic") return 0;   //Converts membership into numeric rank
    if (t == "premium") return 1;
    if (t == "gold") return 2;
    return -1;    //Invalid membership
}
}

inline void registerStudentRoutes(crow::SimpleApp& app) {


    //Get all active borrows for this user
    CROW_ROUTE(app, "/api/student/borrows").methods(crow::HTTPMethod::Get)   //Get Borrowed Books
    ([](const crow::request& req) {
        SessionData s = getSession(req);   // get loggin
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");  // loggin fail

        Database* db = Database::getInstance();  //Get database connection
        // Fetch data from database
        DBResult rows = db->query(
            "SELECT t.id, t.book_id, t.borrow_date, t.due_date, t.return_date, t.fine, t.status,"
            " b.title, b.author, b.book_type "
            "FROM transactions t JOIN books b ON t.book_id=b.id "  //Combine two tables to see book name with author nm
            "WHERE t.user_id=" + std::to_string(s.userId)
            + " ORDER BY t.borrow_date DESC" // Most recent borrows first
        );

        std::vector<crow::json::wvalue> list;    //Creates an empty list to hold JSON objects
        for (auto& row : rows) { //Loops through each database row one by one
            crow::json::wvalue item;
            item["id"]         = std::stoi(row["id"]);
            item["bookId"]     = std::stoi(row["book_id"]);   //converts string  to integer 
            item["title"]      = row["title"];   //add title name
            item["author"]     = row["author"];
            item["bookType"]   = row["book_type"];
            item["borrowDate"] = row["borrow_date"];
            item["dueDate"]    = row["due_date"];
            item["returnDate"] = row["return_date"];
            item["fine"]       = std::stod(row["fine"]);  // converts string to decimal 
            item["status"]     = row["status"];
            list.push_back(std::move(item));    //Add each book to list
        }
        crow::json::wvalue resp;
        resp["borrows"] = std::move(list);
        crow::response res(200, resp.dump());  //Send success response
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // Borrow a book using Facade Pattern
    // Executes LibraryFacade which handles ALL validation + DB updates
   
    CROW_ROUTE(app, "/api/student/borrow").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");

        auto body = crow::json::load(req.body); //read req book body
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");  //Parses the request body as JSON. If the body isn't valid JSON → return 400 (Bad Request)

        int bookId = body["bookId"].i(); // Parse book ID from request body

        //FACADE PATTERN: Create and execute LibraryFacade
        Database* db = Database::getInstance();
        LibraryFacade facade(db); // Create the facade object
        FacadeResult result = facade.borrowBook(s.userId, bookId); // Execute via facade does EVERYTHING (check availability,check membership limit, insert transaction,update book stock) 
        crow::json::wvalue resp;
        resp["success"] = result.success;
        resp["message"] = result.message;
        if (result.success) resp["transactionId"] = result.id;
        return crow::response(result.success ? 200 : 400, resp.dump());
    });

    // Return a borrowed book

    CROW_ROUTE(app, "/api/student/return").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        int txId = body["transactionId"].i(); // Parse transaction ID

        // The Facade also triggers Observer Pattern (waitlist notifications)
        Database* db = Database::getInstance();
        LibraryFacade facade(db);             // Create the facade
        FacadeResult result = facade.returnBook(txId); // Execute: return book + notify waitlist

        crow::json::wvalue resp;
        resp["success"] = result.success;
        resp["message"] = result.message;
        return crow::response(result.success ? 200 : 400, resp.dump());
    });

   
    // seat viewing + booked seats
    
    CROW_ROUTE(app, "/api/student/seats").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");

        std::string date = req.url_params.get("date") ? req.url_params.get("date") : "";  //get data frm url
        Database* db = Database::getInstance();

        // Get all bookings for the requested date (or today)
        std::string dateFilter = date.empty() ? "date('now')" : ("'" + db->escape(date) + "'");
        DBResult booked = db->query(
            "SELECT seat_number, room, time_slot, user_id FROM seat_bookings "
            "WHERE booking_date=" + dateFilter + " AND status='confirmed'"
        );

        // My bookings 
        DBResult myBookings = db->query(
            "SELECT id, seat_number, room, booking_date, time_slot, status "
            "FROM seat_bookings WHERE user_id=" + std::to_string(s.userId)
            + " ORDER BY booking_date DESC"
        );

        std::vector<crow::json::wvalue> bookedList, myList;
        for (auto& r : booked) {
            crow::json::wvalue item;
            item["seatNumber"] = r["seat_number"];
            item["room"]       = r["room"];
            item["timeSlot"]   = r["time_slot"];
            item["isMe"]       = (r["user_id"] == std::to_string(s.userId));
            bookedList.push_back(std::move(item));
        }
        for (auto& r : myBookings) {
            crow::json::wvalue item;
            item["id"]          = std::stoi(r["id"]);
            item["seatNumber"]  = r["seat_number"];
            item["room"]        = r["room"];
            item["bookingDate"] = r["booking_date"];
            item["timeSlot"]    = r["time_slot"];
            item["status"]      = r["status"];
            myList.push_back(std::move(item));
        }

        crow::json::wvalue resp;
        resp["booked"]     = std::move(bookedList);
        resp["myBookings"] = std::move(myList);
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    
    // Reserve a seat
  
    CROW_ROUTE(app, "/api/student/seats/book").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        std::string seat     = body["seatNumber"].s();
        std::string room     = body["room"].s();
        std::string date     = body["date"].s();
        std::string timeSlot = body["timeSlot"].s();
          // if seat is avialable or not
        if (seat.empty() || date.empty() || timeSlot.empty()) {
            return crow::response(400, "{\"error\":\"Seat, date, and time slot required\"}");
        }

        Database* db = Database::getInstance();
        // Check if this seat+time is already taken
        DBRow existing = db->queryOne(
            "SELECT id FROM seat_bookings WHERE seat_number='" + db->escape(seat)
            + "' AND room='" + db->escape(room)
            + "' AND booking_date='" + db->escape(date)
            + "' AND time_slot='" + db->escape(timeSlot)
            + "' AND status='confirmed'"
        );
        if (!existing.empty()) {
            return crow::response(409, "{\"error\":\"This seat is already booked for that time\"}");
        }

        bool ok = db->execute(
            "INSERT INTO seat_bookings (user_id, seat_number, room, booking_date, time_slot) VALUES ("
            + std::to_string(s.userId) + ",'" + db->escape(seat) + "','"
            + db->escape(room) + "','" + db->escape(date) + "','" + db->escape(timeSlot) + "')"
        );
        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["message"] = ok ? "Seat " + seat + " booked for " + timeSlot : "Booking failed.";
        return crow::response(ok ? 201 : 500, resp.dump());
    });

  
    //Get this user's notifications
   
    CROW_ROUTE(app, "/api/student/notifications").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");

        Database* db = Database::getInstance();
        DBResult rows = db->query(
            "SELECT id, message, is_read, created_at FROM notifications "
            "WHERE user_id=" + std::to_string(s.userId)
            + " ORDER BY created_at DESC LIMIT 20"
        );
        // Mark all as read now that user is viewing them
        db->execute(
            "UPDATE notifications SET is_read=1 WHERE user_id=" + std::to_string(s.userId)
        );

        std::vector<crow::json::wvalue> list;
        for (auto& r : rows) {
            crow::json::wvalue item;
            item["id"]        = std::stoi(r["id"]);
            item["message"]   = r["message"];
            item["isRead"]    = r["is_read"] == "1";
            item["createdAt"] = r["created_at"];
            list.push_back(std::move(item));
        }
        crow::json::wvalue resp;
        resp["notifications"] = std::move(list);
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // waitlist for a book

    CROW_ROUTE(app, "/api/student/waitlist").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");
        int bookId = body["bookId"].i();

        Database* db = Database::getInstance();
        // Check if already on waitlist
        DBRow existing = db->queryOne(
            "SELECT id FROM waitlist WHERE user_id=" + std::to_string(s.userId)
            + " AND book_id=" + std::to_string(bookId) + " AND status='waiting'"
        );
        if (!existing.empty())
            return crow::response(409, "{\"error\":\"Already on waitlist for this book\"}");

        // Get next queue position
        DBRow posRow = db->queryOne(
            "SELECT COUNT(*)+1 AS pos FROM waitlist WHERE book_id="
            + std::to_string(bookId) + " AND status='waiting'"
        );
        int pos = posRow.empty() ? 1 : std::stoi(posRow["pos"]);

        bool ok = db->execute(
            "INSERT INTO waitlist (user_id, book_id, position) VALUES ("
            + std::to_string(s.userId) + "," + std::to_string(bookId)
            + "," + std::to_string(pos) + ")"
        );
        crow::json::wvalue resp;
        resp["success"]  = ok;
        resp["position"] = pos;
        resp["message"]  = ok ? "Added to waitlist at position #" + std::to_string(pos) : "Failed.";
        return crow::response(ok ? 201 : 500, resp.dump());
    });

    
    // membership-plans

    CROW_ROUTE(app, "/api/student/membership-plans").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");

        Database* db = Database::getInstance();
        DBResult rows = db->query(
            "SELECT tier_name, max_books, borrow_days, fine_per_day, monthly_fee "
            "FROM memberships ORDER BY monthly_fee ASC"
        );
        std::vector<crow::json::wvalue> list;
        for (auto& r : rows) {
            crow::json::wvalue item;
            item["tierName"]    = r["tier_name"];
            item["maxBooks"]    = std::stoi(r["max_books"]);
            item["borrowDays"]  = std::stoi(r["borrow_days"]);
            item["finePerDay"]  = std::stod(r["fine_per_day"]);
            item["monthlyFee"]  = std::stod(r["monthly_fee"]);
            list.push_back(std::move(item));
        }
        crow::json::wvalue resp;
        resp["plans"] = std::move(list);
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    
    // billing .......
    
    CROW_ROUTE(app, "/api/student/billing").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");

        Database* db = Database::getInstance();
        DBResult rows = db->query(
            "SELECT id, target_tier, amount_bdt, status, created_at, paid_at, description "
            "FROM membership_bills WHERE user_id=" + std::to_string(s.userId)
            + " ORDER BY created_at DESC"
        );
        std::vector<crow::json::wvalue> list;
        double pendingTotal = 0.0;
        for (auto& r : rows) {
            crow::json::wvalue item;
            item["id"]          = std::stoi(r["id"]);
            item["targetTier"]  = r["target_tier"];
            item["amountBdt"]   = std::stod(r["amount_bdt"]);
            item["status"]      = r["status"];
            item["createdAt"]   = r["created_at"];
            item["paidAt"]      = r["paid_at"];
            item["description"] = r["description"];
            list.push_back(std::move(item));
            if (r["status"] == "pending") pendingTotal += std::stod(r["amount_bdt"]);
        }
        crow::json::wvalue resp;
        resp["bills"]        = std::move(list);
        resp["pendingTotal"] = pendingTotal;
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // ROUTE: POST /api/student/billing/pay
    // Simulates a payment gateway transaction.
    CROW_ROUTE(app, "/api/student/billing/pay").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");

        auto body = crow::json::load(req.body);
        if (!body || !body.has("billId"))
            return crow::response(400, "{\"error\":\"billId required\"}");

        int billId = body["billId"].i();
        Database* db = Database::getInstance();
        DBRow bill = db->queryOne(
            "SELECT id, target_tier, amount_bdt, status FROM membership_bills WHERE id="
            + std::to_string(billId) + " AND user_id=" + std::to_string(s.userId)
        );
        if (bill.empty())
            return crow::response(404, "{\"error\":\"Invoice not found\"}");
        if (bill["status"] != "pending")
            return crow::response(400, "{\"error\":\"Invoice is already paid\"}");

        std::string tier = bill["target_tier"];
        double amt = std::stod(bill["amount_bdt"]);

        // Mark invoice as paid (simulate payment gateway success)
        bool ok = db->execute(
            "UPDATE membership_bills SET status='paid', paid_at=datetime('now') "
            "WHERE id=" + std::to_string(billId)
        );
        if (!ok) return crow::response(500, "{\"error\":\"Payment failed\"}");

        // Update user membership tier
        db->execute(
            "UPDATE users SET membership='" + db->escape(tier) + "' WHERE id=" + std::to_string(s.userId)
        );

        // Add a notification for the student
        std::string msg = "[Billing] Payment successful! Your membership is now " + tier + ".";
        db->execute(
            "INSERT INTO notifications (user_id, message, is_read) VALUES ("
            + std::to_string(s.userId) + ", '" + db->escape(msg) + "', 0)"
        );

        crow::json::wvalue resp;
        resp["success"] = true;
        resp["message"] = "Payment successful!";
        return crow::response(200, resp.dump());
    });

    
    // purchase membership Request upgrade (creates due bill)
    
    CROW_ROUTE(app, "/api/student/membership/purchase").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData s = getSession(req);
        if (s.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");
        if (s.role != "student")
            return crow::response(403, "{\"error\":\"Only students can purchase membership\"}");

        auto body = crow::json::load(req.body);
        if (!body || !body.has("tier"))
            return crow::response(400, "{\"error\":\"Invalid JSON: tier required\"}");

        std::string target = body["tier"].s();
        if (membershipTierRank(target) < 1)  
            return crow::response(400, "{\"error\":\"Choose premium or gold\"}");

        Database* db = Database::getInstance();
        DBRow user = db->queryOne(
            "SELECT membership FROM users WHERE id=" + std::to_string(s.userId)
        );
        if (user.empty())
            return crow::response(404, "{\"error\":\"User not found\"}");

        std::string current = user["membership"];
        if (membershipTierRank(target) <= membershipTierRank(current)) {
            return crow::response(400, "{\"error\":\"You already have this tier or higher\"}");
        }

        DBRow pending = db->queryOne(
            "SELECT id FROM membership_bills WHERE user_id=" + std::to_string(s.userId)
            + " AND target_tier='" + db->escape(target) + "' AND status='pending'"
        );
        if (!pending.empty())
            return crow::response(409, "{\"error\":\"You already have a pending invoice for this tier\"}");

        DBRow plan = db->queryOne(
            "SELECT monthly_fee FROM memberships WHERE tier_name='" + db->escape(target) + "'"
        );
        if (plan.empty())
            return crow::response(400, "{\"error\":\"Unknown membership tier\"}");

        double fee = std::stod(plan["monthly_fee"]);
        if (fee <= 0.0)
            return crow::response(400, "{\"error\":\"This tier has no fee\"}");

        std::string desc = "Membership upgrade to " + target;
        bool ok = db->execute(
            //Create invoice
            "INSERT INTO membership_bills (user_id, target_tier, amount_bdt, status, description) VALUES ("
            + std::to_string(s.userId) + ",'" + db->escape(target) + "',"
            + std::to_string(fee) + ",'pending','" + db->escape(desc) + "')"
        );
        if (!ok) return crow::response(500, "{\"error\":\"Could not create invoice\"}");

        int billId = db->lastInsertId();
        std::string msg = "[Billing] Invoice #" + std::to_string(billId) + ": Upgrade to "
            + target + " — " + std::to_string((int)fee)
            + " BDT due. Pay at the library; staff will mark your payment when received.";
        db->execute(
            //send notification message
            "INSERT INTO notifications (user_id, message, is_read) VALUES ("
            + std::to_string(s.userId) + ", '" + db->escape(msg) + "', 0)"
        );

        crow::json::wvalue resp;
        resp["success"]    = true;
        resp["billId"]     = billId;
        resp["amountBdt"]  = fee;
        resp["targetTier"] = target;
        resp["message"]    = "Invoice created. Please pay " + std::to_string((int)fee)
            + " BDT to activate your " + target + " membership.";
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });
}
