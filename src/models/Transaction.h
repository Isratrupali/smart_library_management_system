// PURPOSE: Data structure for borrow/return transactions.
// Used by the Facade Pattern (LibraryFacade).

#pragma once
#include <string>   

// STRUCT: Transaction- Records one borrow or return event.

struct Transaction //data studecture
{
    int         id          = 0;          // Unique transaction ID (auto-assigned by DB)
    int         userId      = 0;          // Which user borrowed the book
    int         bookId      = 0;          // Which book was borrowed
    std::string borrowDate  = "";         // Date/time when borrowing happened
    std::string dueDate     = "";         // Date by which the book must be returned
    std::string returnDate  = "";         // Actual return date (empty if not yet returned)
    double      fine        = 0.0;        // Overdue fine charged on this transaction
    std::string status      = "active";   // Status: "active", "returned", or "overdue"

    // Joined fields ( populated by JOIN queries for display)
    std::string userName    = "";        
    std::string bookTitle   = "";    
    std::string bookAuthor  = "";   


    // isOverdue- Returns true if status is "overdue"

    bool isOverdue() const //const = doesnt modify data
    {
        return status == "overdue";
    }

    // isActive- Returns true if borrow is still open

    bool isActive() const {
        return status == "active" || status == "overdue"; // Both mean book not yet returned
    }

    // isReturned- True when book has been given back

    bool isReturned() const {
        return status == "returned"; // Book has been returned
    }
};
