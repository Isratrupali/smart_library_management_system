// Provide a unified, simple interface for complex operations.
#pragma once
#include "../database/Database.h"
#include "../patterns/UserFactory.h"
#include "../patterns/Observer.h"
#include <string>
#include <ctime>    // time() for calculating due dates


struct FacadeResult {
    bool        success = false; // Did the operation succeed?
    std::string message = "";
    int         id      = 0;     // ID of created record (if any)
};


class LibraryFacade {
private:
    Database* db; 

public:
    LibraryFacade(Database* database) : db(database) {}

    // borrowBook:
    
    FacadeResult borrowBook(int userId, int bookId) {
        FacadeResult result;

        // Step 1: Get user info from DB
        DBRow user = db->queryOne(
            "SELECT role, is_suspended, fine_balance, membership FROM users WHERE id="
            + std::to_string(userId)
        );
        if (user.empty()) { result.message = "User not found."; return result; }

        std::string role       = user["role"];
        int suspended          = std::stoi(user["is_suspended"]);
        double fineBalance     = std::stod(user["fine_balance"]);

        // Step 2: Check account suspension
        if (suspended) {
            result.message = "Your account is suspended. Contact the librarian.";
            return result;
        }

        // Step 3: Apply Factory Pattern- get role-specific rules
        auto userObj = UserFactory::create(role);

        // Students with fine > 50 BDT cannot borrow new books
        if (role == "student" && fineBalance > 50.0) {
            result.message = "Pay your fine of " + std::to_string((int)fineBalance)
                           + " BDT before borrowing.";
            return result;
        }

        // Step 4: Count current active borrows for this user
        DBRow countRow = db->queryOne(
            "SELECT COUNT(*) as cnt FROM transactions WHERE user_id="
            + std::to_string(userId) + " AND status='active'"
        );
        int currentBorrows = countRow.empty() ? 0 : std::stoi(countRow["cnt"]);

        // Check Factory-defined limit
        if (currentBorrows >= userObj->getMaxBooks()) {
            result.message = "Borrow limit reached ("
                           + std::to_string(userObj->getMaxBooks())
                           + " books max for " + userObj->getDisplayRole() + ").";
            return result;
        }

        // Step 5: Check book availability
        DBRow book = db->queryOne(
            "SELECT title, available_copies FROM books WHERE id=" + std::to_string(bookId)
        );
        if (book.empty()) { result.message = "Book not found."; return result; }

        int available = std::stoi(book["available_copies"]);
        std::string bookTitle = book["title"];

        if (available <= 0) {
            result.message = "No copies of \"" + bookTitle + "\" available. Join the waitlist.";
            return result;
        }

        // Step 6: Prevent duplicate borrow of same book
        DBRow dupCheck = db->queryOne(
            "SELECT id FROM transactions WHERE user_id=" + std::to_string(userId)
            + " AND book_id=" + std::to_string(bookId) + " AND status='active'"
        );
        if (!dupCheck.empty()) {
            result.message = "You already have this book borrowed.";
            return result;
        }

        // Step 7: Calculate due date using Factory rule (getBorrowDays)
        time_t now     = time(nullptr);
        int borrowDays = userObj->getBorrowDays();
        time_t dueTime = now + (borrowDays * 24 * 60 * 60);
        char dueBuf[20];
        strftime(dueBuf, sizeof(dueBuf), "%Y-%m-%d", localtime(&dueTime));
        std::string dueDate(dueBuf);

        // Step 8: Insert the transaction record
        bool ok = db->execute(
            "INSERT INTO transactions (user_id, book_id, due_date, status) VALUES ("
            + std::to_string(userId) + ", "
            + std::to_string(bookId) + ", '"
            + dueDate + "', 'active')"
        );
        if (!ok) { result.message = "Database error. Please try again."; return result; }

        result.id = db->lastInsertId(); // Get the new transaction's ID

        // Step 9: Decrement available_copies in books table
        db->execute(
            "UPDATE books SET available_copies=available_copies-1, borrow_count=borrow_count+1"
            " WHERE id=" + std::to_string(bookId)
        );

        result.success = true;
        result.message = "Successfully borrowed \"" + bookTitle
                       + "\". Due: " + dueDate + " (" + std::to_string(borrowDays) + " days).";
        return result;
    }

   
    // returnBook:
    FacadeResult returnBook(int transactionId) {
        FacadeResult result;

        //Step 1: Get the borrow record + join book info
        DBRow tx = db->queryOne(
            "SELECT t.user_id, t.book_id, t.due_date, b.title "
            "FROM transactions t JOIN books b ON t.book_id=b.id "
            "WHERE t.id=" + std::to_string(transactionId) + " AND t.status='active'"
        );
        if (tx.empty()) {
            result.message = "Active borrow record not found.";
            return result;
        }

        int         userId    = std::stoi(tx["user_id"]);
        int         bookId    = std::stoi(tx["book_id"]);
        std::string bookTitle = tx["title"];
        std::string dueDate   = tx["due_date"];

        // Step 2: Calculate overdue fine
        DBRow fineRow = db->queryOne(
            "SELECT MAX(0, CAST((julianday('now') - julianday('" + db->escape(dueDate)
            + "')) AS INTEGER)) AS days_overdue"
        );
        int daysOverdue = 0;
        if (!fineRow.empty() && !fineRow["days_overdue"].empty()) {
            daysOverdue = std::stoi(fineRow["days_overdue"]);
        }
        double fine = daysOverdue * 5.0; // 5 BDT per overdue day

        // Step 3: Mark transaction as returned
        std::string newStatus = (daysOverdue > 0) ? "returned" : "returned";
        db->execute(
            "UPDATE transactions SET return_date=datetime('now'), fine="
            + std::to_string(fine) + ", status='returned' WHERE id=" + std::to_string(transactionId)
        );

        // Step 4: Restore available_copies
        db->execute(
            "UPDATE books SET available_copies=available_copies+1 WHERE id=" + std::to_string(bookId)
        );

        // Step 5: Add fine to user's balance if overdue
        if (fine > 0) {
            db->execute(
                "UPDATE users SET fine_balance=fine_balance+" + std::to_string(fine)
                + " WHERE id=" + std::to_string(userId)
            );
        }

        // Step 6: Trigger Observer- notify waitlisted users
        int notified = WaitlistNotifier::notifyWaitlist(bookId, bookTitle, db);

        result.success = true;
        result.message = "\"" + bookTitle + "\" returned successfully.";
        if (fine > 0) result.message += " Fine: " + std::to_string((int)fine) + " BDT added.";
        if (notified > 0) result.message += " " + std::to_string(notified) + " user(s) notified.";
        return result;
    }
};
