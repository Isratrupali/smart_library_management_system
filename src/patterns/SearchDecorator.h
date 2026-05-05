// DESIGN PATTERN: DECORATOR
// The Decorator Pattern adds search filters to the book list
//   Decorator that "wraps" the previous filter.

#pragma once                    // Prevent double-inclusion

#include "../models/Book.h"     // BookList (vector<Book>)
#include <string>               
#include <memory>               
#include <algorithm>            // (lowercase conversion)
#include <cctype>               // (char to lowercase)


// COMPONENT - IBookFilter Interface all filters implement

class IBookFilter {
public:
    virtual BookList filter(const BookList& books) = 0;
    virtual ~IBookFilter() = default;
};

class BaseFilter : public IBookFilter {
public:
    BookList filter(const BookList& books) override {
        return books;  
    }
};


class GenreFilter : public IBookFilter {
private:
    std::unique_ptr<IBookFilter> inner; 
    std::string genre;               

public:
    GenreFilter(std::unique_ptr<IBookFilter> inner, const std::string& genre)
        : inner(std::move(inner)),  
          genre(genre) {}          

    BookList filter(const BookList& books) override {
        BookList innerResult = inner->filter(books); 
        BookList result; 
        for (const Book& b : innerResult) {
            if (b.genre == genre) {     
                result.push_back(b); 
            }
        }
        return result;                 
    }
};

class AvailableFilter : public IBookFilter {
private:
    std::unique_ptr<IBookFilter> inner; 

public:
    explicit AvailableFilter(std::unique_ptr<IBookFilter> inner)
        : inner(std::move(inner)) {} 

    BookList filter(const BookList& books) override {
        BookList innerResult = inner->filter(books); 
        BookList result;                             
        for (const Book& b : innerResult) {
            if (b.availableCopies > 0) {            
                result.push_back(b);             
            }
        }
        return result;                           
    }
};

class TypeFilter : public IBookFilter {
private:
    std::unique_ptr<IBookFilter> inner;
    std::string bookType;            

public:
    TypeFilter(std::unique_ptr<IBookFilter> inner, const std::string& bookType)
        : inner(std::move(inner)),  
          bookType(bookType) {}     

    BookList filter(const BookList& books) override {
        BookList innerResult = inner->filter(books);   
        for (const Book& b : innerResult) {
            if (b.bookType == bookType) {              
                result.push_back(b);       
            }
        }
        return result;  
    }
};


// DECORATOR 4: KeywordFilter (Keeps books where title, author, or ISBN contains the keyword.)

class KeywordFilter : public IBookFilter {
private:
    std::unique_ptr<IBookFilter> inner; // The wrapped inner filter
    std::string keyword;                // The search keyword ("algorithm")

    // Convert a string to all lowercase for case-insensitive comparison
    std::string toLower(const std::string& s) const {
        std::string result = s;                // Copy the string to modify
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); }); // Lowercase each char
        return result;
    }

public:
    KeywordFilter(std::unique_ptr<IBookFilter> inner, const std::string& keyword)
        : inner(std::move(inner)),  // Take ownership of inner filter
          keyword(toLower(keyword)) {} // Store keyword in lowercase for comparison

    BookList filter(const BookList& books) override {
        BookList innerResult = inner->filter(books);    // Run inner layers first
        BookList result;                                 // Books matching the keyword
        for (const Book& b : innerResult) {
            // Convert each field to lowercase before comparing
            bool titleMatch  = toLower(b.title).find(keyword)  != std::string::npos;
            bool authorMatch = toLower(b.author).find(keyword) != std::string::npos;
            bool isbnMatch   = toLower(b.isbn).find(keyword)   != std::string::npos;
            bool descMatch   = toLower(b.description).find(keyword) != std::string::npos;

            // Include the book if any field contains the keyword
            if (titleMatch || authorMatch || isbnMatch || descMatch) {
                result.push_back(b);    // Match found — include this book
            }
        }
        return result;                  // Return keyword-matching books
    }
};


// DECORATOR 5: DepartmentFilter (Keeps only books belonging to a specific academic department.)

class DepartmentFilter : public IBookFilter {
private:
    std::unique_ptr<IBookFilter> inner; // The wrapped inner filter
    std::string department;             // The department to filter ("CSE")

public:
    DepartmentFilter(std::unique_ptr<IBookFilter> inner, const std::string& dept)
        : inner(std::move(inner)), department(dept) {}

    BookList filter(const BookList& books) override {
        BookList innerResult = inner->filter(books);    // Run inner layers first
        BookList result;
        for (const Book& b : innerResult) {
            if (b.department == department) {           // Department match?
                result.push_back(b);                    // Include this book
            }
        }
        return result;
    }
};


//SearchFilterBuilder Convenience class to build a Decorator chain from API parameters.
// Makes it easy to construct complex filter combinations.

class SearchFilterBuilder {
private:
    std::unique_ptr<IBookFilter> chain; // The filter chain being built

public:
    // always start with a BaseFilter (returns everything)
    SearchFilterBuilder() : chain(std::make_unique<BaseFilter>()) {}

    // Add a keyword filter on top of the current chain
    SearchFilterBuilder& withKeyword(const std::string& kw) {
        if (!kw.empty()) {  // Only add this filter if a keyword was actually provided
            chain = std::make_unique<KeywordFilter>(std::move(chain), kw);
        }
        return *this;       // Return reference to self for method chaining
    }

    // Add a genre filter on top of the current chain
    SearchFilterBuilder& withGenre(const std::string& genre) {
        if (!genre.empty() && genre != "all") { // Ignore "all" (no filter needed)
            chain = std::make_unique<GenreFilter>(std::move(chain), genre);
        }
        return *this;       // Method chaining — allows: builder.withGenre().withKeyword()
    }

    // Add an availability filter — only show books with copies in stock
    SearchFilterBuilder& onlyAvailable() {
        chain = std::make_unique<AvailableFilter>(std::move(chain));
        return *this;       // Return self for chaining
    }

    // Add a book type filter (physical / ebook / audio)
    SearchFilterBuilder& withType(const std::string& type) {
        if (!type.empty() && type != "all") {   // Ignore "all" (no filter needed)
            chain = std::make_unique<TypeFilter>(std::move(chain), type);
        }
        return *this;       // Method chaining
    }

    // Add a department filter
    SearchFilterBuilder& withDepartment(const std::string& dept) {
        if (!dept.empty() && dept != "all") {   // Ignore "all" (no filter needed)
            chain = std::make_unique<DepartmentFilter>(std::move(chain), dept);
        }
        return *this;       // Method chaining
    }

    // build: Finalize and return the complete filter chain
    std::unique_ptr<IBookFilter> build() {
        return std::move(chain); // Transfer ownership of the chain to the caller
    }
};
