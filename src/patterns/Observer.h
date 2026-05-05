// OBSERVER (Publish-Subscribe)
// PURPOSE: Notify waitlisted users when a returned book is available.

#pragma once
#include "../database/Database.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

// IBookObserver — every observer must implement update()

class IBookObserver {
public:
    // Called by Subject when a book becomes available
    virtual void update(int bookId, const std::string& message) = 0;
    virtual ~IBookObserver() = default; // Virtual destructor for safe polymorphism
};


// CONCRETE OBSERVER UserNotificationObserver - Writes a DB notification row for one specific user

class UserNotificationObserver : public IBookObserver {
private:
    int       userId; // Which user gets this notification
    Database* db;     // DB — for writing the notification row

public:
    UserNotificationObserver(int uid, Database* database)
        : userId(uid), db(database) {} // Store user ID and DB reference

    void update(int bookId, const std::string& message) override {
        // INSERT a notification row so the user sees it on their dashboard
        db->execute(
            "INSERT INTO notifications (user_id, message, is_read) VALUES ("
            + std::to_string(userId) + ", '"
            + db->escape(message) + "', 0)" // is_read=0 means unread
        );
        // Mark waitlist entry as 'notified' so we don't notify again
        db->execute(
            "UPDATE waitlist SET status='notified' WHERE user_id="
            + std::to_string(userId) + " AND book_id=" + std::to_string(bookId)
            + " AND status='waiting'"
        );
        std::cout << "[Observer] User " << userId << " notified for book " << bookId << "\n";
    }
};


// SUBJECT- BookSubject holds observer list and fires events

class BookSubject {
private:
    std::vector<IBookObserver*> observers; // All registered observers

public:
    void attach(IBookObserver* o) { observers.push_back(o); } // Add observer

    void detach(IBookObserver* o) { // Remove observer
        auto it = std::find(observers.begin(), observers.end(), o);
        if (it != observers.end()) observers.erase(it);
    }

    // notifyAll: update() on every registered observer
    void notifyAll(int bookId, const std::string& message) {
        for (IBookObserver* o : observers) o->update(bookId, message);
        std::cout << "[Observer] Notified " << observers.size() << " users\n";
    }
    int count() const { return (int)observers.size(); }
};

// WaitlistNotifier — queries DB and fires Observer notifications
// Called by LibraryFacade after a book is successfully returned.

class WaitlistNotifier {
public:
    static int notifyWaitlist(int bookId, const std::string& bookTitle, Database* db) {
        DBResult waiters = db->query(
            "SELECT user_id, position FROM waitlist WHERE book_id="
            + std::to_string(bookId) + " AND status='waiting' ORDER BY position ASC LIMIT 5"
        );
        if (waiters.empty()) return 0; 

        for (auto& row : waiters) {
            int uid = std::stoi(row["user_id"]); 
            int pos = std::stoi(row["position"]); 

            std::string msg = (pos == 1)
                ? "\"" + bookTitle + "\" is now available! You are FIRST — borrow within 48h!"
                : "\"" + bookTitle + "\" is available. You are #" + std::to_string(pos) + " in queue.";

            UserNotificationObserver obs(uid, db); 
            obs.update(bookId, msg);  
        }
        return (int)waiters.size(); 
    }
};
