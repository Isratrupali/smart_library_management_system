// Defines the Book data structure used throughout the app.
//The Decorator and Strategy patterns operate on Book objects.

#pragma once
#include <string>    
#include <vector>    //for lists of books

//Book — A plain data container for one book record.
struct Book {
    int         id              = 0;          // Unique book ID from DB
    std::string title           = "";         // Book title 
    std::string author          = "";         // Author name
    std::string isbn            = "";         // ISBN numb
    std::string genre           = "";         // Genre
    std::string department      = "";         // Academic dept
    std::string bookType        = "physical"; // Type: "physical", "ebook", or "audio"
    int         totalCopies     = 1;          // Total copies owned by the library
    int         availableCopies = 1;          // Copies currently available to borrow
    int         publishedYear   = 2000;       // Year of published
    std::string description     = "";         //summary of the book
    std::string coverUrl        = "";         // URL to the book cover
    std::string ebookUrl        = "";         // ebook URL
    std::string audioUrl        = "";         // audio URL
    std::string ebookUrlsJson   = "[]";       // JSON array for multi-part PDFs
    std::string audioUrlsJson   = "[]";       // JSON array for multi-part audio
    int         borrowCount     = 0;          // Total times borrowed 
    std::string bookCondition   = "good";     // Physical condition
    std::string createdAt       = "";         // Timestamp when added to catalog

    // isAvailable: Returns true if at least one copy can be borrowed
    bool isAvailable() const {
        return availableCopies > 0; // If availableCopies is 0, no copy is available
    }

    // isEbook: Returns true if this book has a digital ebook
    bool isEbook() const {
        return bookType == "ebook"; // Check the type field for ebook
    }

    // isAudio: Returns true if this book has an audiobook 
    bool isAudio() const {
        return bookType == "audio"; // Check the type field for audio
    }

    // getAvailabilityText: Returns a human-readable availability status string
    std::string getAvailabilityText() const {
        if (availableCopies <= 0) return "Not Available"; // All copies borrowed
        return "Available (" + std::to_string(availableCopies) + ")"; // Show count
    }
};

using BookList = std::vector<Book>; // BookList is just a vector of Book structs
