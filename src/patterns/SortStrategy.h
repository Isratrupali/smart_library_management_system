// DESIGN PATTERN: STRATEGY (allows swapping the book sorting algorithm)
//   The user picks a sort option (title/author/year/popularity),
//   and the correct strategy is applied to the book list.


#pragma once                    
#include "../models/Book.h"     // BookList type (vector<Book>)
#include <string>               
#include <algorithm>            // for sorting algorithms
#include <memory>               // pointer for strategy objects


// ISortStrategy — Interface all sort strategies must implement
//
class ISortStrategy {
public:
    // sort: Apply this strategy's sorting algorithm to the book list.
    // every subclass MUST implement this.
    virtual void sort(BookList& books) = 0;

    // ensures proper cleanup when deleting via base pointer
    virtual ~ISortStrategy() = default;
};


// STRATEGY 1: TitleSort (Sorts books alphabetically by title (A → Z))

class TitleSort : public ISortStrategy {
public:
    void sort(BookList& books) override {
        // std::sort with a lambda comparator
        // Lambda takes two Book references (a, b) and returns true if a < b
        std::sort(books.begin(), books.end(), [](const Book& a, const Book& b) {
            return a.title < b.title;   // Alphabetical: "Algorithms" before "Clean Code"
        });
    }
};


//STRATEGY 2: AuthorSort (Sorts books alphabetically by author name (A → Z))

class AuthorSort : public ISortStrategy {
public:
    void sort(BookList& books) override {
        std::sort(books.begin(), books.end(), [](const Book& a, const Book& b) {
            return a.author < b.author; // Alphabetical by author 
        });
    }
};


// STRATEGY 3: YearSort (Sorts books by publication year — newest first)

class YearSort : public ISortStrategy {
public:
    void sort(BookList& books) override {
        std::sort(books.begin(), books.end(), [](const Book& a, const Book& b) {
            return a.publishedYear > b.publishedYear; // Descending
        });
    }
};


// STRATEGY 4: PopularitySort (Sorts books by borrow_count — most borrowed )

class PopularitySort : public ISortStrategy {
public:
    void sort(BookList& books) override {
        std::sort(books.begin(), books.end(), [](const Book& a, const Book& b) {
            return a.borrowCount > b.borrowCount; // Descending: 55 borrows before 10
        });
    }
};


// STRATEGY 5: AvailabilitySort () Sorts books — available first, then unavailable)

class AvailabilitySort : public ISortStrategy {
public:
    void sort(BookList& books) override {
        std::sort(books.begin(), books.end(), [](const Book& a, const Book& b) {
            // available > 0 means the book is in stock
            // more copies = rank higher
            return a.availableCopies > b.availableCopies; // Books in stock come first
        });
    }
};


// CONTEXT - BookSorter Holds and applies a sorting strategy

class BookSorter {
private:
    // The current sorting strategy starts as nullptr (no sort = original order)
    std::unique_ptr<ISortStrategy> strategy; // pointer manages strategy 

public:
    
    // setStrategy: Install a new sorting strategy by name.
    // Takes a strategy name string (from API query parameter).
    
    void setStrategy(const std::string& strategyName) {
        if (strategyName == "title") {
            // User wants A-Z title sort
            strategy = std::make_unique<TitleSort>();
        }
        else if (strategyName == "author") {
            // User wants A-Z author sort
            strategy = std::make_unique<AuthorSort>();
        }
        else if (strategyName == "year") {
            // User wants newest-first sort
            strategy = std::make_unique<YearSort>();
        }
        else if (strategyName == "popular") {
            // User wants most-borrowed-first sort
            strategy = std::make_unique<PopularitySort>();
        }
        else if (strategyName == "available") {
            // User wants available books first
            strategy = std::make_unique<AvailabilitySort>();
        }
        else {
            // Unknown sort name default to title sort as a safe fallback
            strategy = std::make_unique<TitleSort>();
        }
    }


    // sort: Apply the current strategy to sort the book list.
    // If no strategy has been set, does nothing (preserves original order).
    
    void sort(BookList& books) {
        if (strategy) {             // Is a strategy installed?
            strategy->sort(books);  // Yes . delegate sorting to the strategy object
        }
        // If no strategy is set, leave the books in their original (DB) order
    }
};
