
// FILE: src/handlers/BookHandler.h
// PURPOSE: API routes for the book catalog.
//          Uses Decorator Pattern (search filters) + Strategy Pattern (sort).

#pragma once
#include "../libs/crow_all.h"           //Crow framework → handles HTTP requests
#include "../database/Database.h"     //Use thr database
#include "../models/Book.h"           //Book structure (data model)
#include "../patterns/SearchDecorator.h"  // Decorator Pattern for filtering
#include "../patterns/SortStrategy.h"     // Strategy Pattern for sorting
#include "../patterns/Observer.h"         // Observer Pattern for waitlist
#include "AuthHandler.h"                  // getSession() Used for login/session checking
#include <string>
#include <vector>
#include <fstream>
#include <cctype>
#include <ctime>
#include <set>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {
inline std::string uploadsBaseDir() {
#ifdef _WIN32
    char buf[MAX_PATH] = {0};
    if (GetModuleFileNameA(nullptr, buf, MAX_PATH)) {
        std::string full(buf);
        std::size_t pos = full.find_last_of("\\/");   //(folder separator)
        if (pos != std::string::npos) return full.substr(0, pos);  //Return folder path
    }
#endif
    return ".";  //Otherwise → current folder
}
inline std::string toLowerExt(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); //Loop each character convert to lowercase
    return s;
}
inline std::string fileExtension(const std::string& name) {
    auto pos = name.find_last_of('.');  //Find last dot
    if (pos == std::string::npos || pos == name.length() - 1) return "";
    return toLowerExt(name.substr(pos));   //Return extension .pdf
}
inline std::string multipartFilename(const crow::multipart::part& filepart)    //Extract filename from uploaded file
{
    try {
        const auto& cd = filepart.get_header_object("Content-Disposition");
        auto it = cd.params.find("filename");
        if (it != cd.params.end()) {
            std::string name = it->second;
            if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
                name = name.substr(1, name.size() - 2);
            auto slash = name.find_last_of("/\\");
            if (slash != std::string::npos) name = name.substr(slash + 1);
            return name;
        }
    } catch (...) {}
    return "upload";
}
inline std::string trimStr(std::string s)   //Remove all spaces
 {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ')) s.pop_back();
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\r' || s[i] == '\n')) i++;
    return s.substr(i);
}

inline std::string serializeMediaListFromRvalue(const crow::json::rvalue& arr) {
    std::vector<crow::json::wvalue> items;
    for (size_t i = 0; i < arr.size(); ++i) //Loop through JSON array
     {
        const auto& it = arr[i];
        if (!it.has("url")) continue;  //Skip invalid items
        std::string url = static_cast<std::string>(it["url"]);
        if (url.empty()) continue;
        std::string label = it.has("label") ? static_cast<std::string>(it["label"]) : "";
        crow::json::wvalue o;
        o["url"] = url;   //store url
        o["label"] = label.empty() ? ("Part " + std::to_string(items.size() + 1)) : label;
        items.push_back(std::move(o));  //Add to list
    }
    crow::json::wvalue w;
    w = std::move(items);
    return w.dump();   //Convert to JSON string
}

inline std::string firstUrlFromJsonArray(const std::string& json)
{
    auto j = crow::json::load(json); 
    if (!j || j.t() != crow::json::type::List || j.size() == 0) return "";  //if invalid return ''
    if (!j[0].has("url")) return "";  
    return static_cast<std::string>(j[0]["url"]);   //Return first URL
}

inline std::string singleItemMediaJson(const std::string& url, const char* defaultLabel) {
    std::vector<crow::json::wvalue> items;
    crow::json::wvalue o;
    o["url"] = url;
    o["label"] = std::string(defaultLabel);
    items.push_back(std::move(o));
    crow::json::wvalue w;
    w = std::move(items);
    return w.dump();
}

inline crow::json::wvalue wvalueMediaListFromStored(const std::string& json, const std::string& fallbackSingle) {
    std::vector<crow::json::wvalue> items;
    auto j = crow::json::load(json);
    if (j && j.t() == crow::json::type::List) {
        for (size_t i = 0; i < j.size(); ++i) {
            const auto& it = j[i];
            if (!it.has("url")) continue;
            std::string url = static_cast<std::string>(it["url"]);
            if (url.empty()) continue;
            std::string label = it.has("label") ? static_cast<std::string>(it["label"]) : "";
            crow::json::wvalue o;
            o["url"] = url;
            o["label"] = label.empty() ? ("Part " + std::to_string(items.size() + 1)) : label;
            items.push_back(std::move(o));
        }
    }
    if (items.empty() && !fallbackSingle.empty()) {
        crow::json::wvalue o;
        o["url"] = fallbackSingle;
        o["label"] = "Read";
        items.push_back(std::move(o));
    }
    crow::json::wvalue w;
    w = std::move(items);
    return w;
}

inline void resolveMediaFieldsFromBody(
    const crow::json::rvalue& body,
    const char* listKey,
    const char* singleKey,
    std::string& outJson,
    std::string& outFirstUrl)
{
    outJson = "[]";   //Default empty
    outFirstUrl = "";
    if (body.has(listKey) && body[listKey].t() == crow::json::type::List) //If multiple URLs exist
    {
        if (body[listKey].size() > 0) {
            outJson = serializeMediaListFromRvalue(body[listKey]);  //Convert to JSON
            outFirstUrl = firstUrlFromJsonArray(outJson);     //Get first URL
            return;
        }
        if (!body.has(singleKey) || std::string(body[singleKey].s()).empty()) 
            return;
    }
    if (body.has(singleKey))   //If single URL
     {
        outFirstUrl = std::string(body[singleKey].s());
        if (!outFirstUrl.empty())
            outJson = singleItemMediaJson(outFirstUrl, "Read");  //Convert to JSON list
    }
}
}


// HELPER: Convert a DBRow into a Book struct

inline Book rowToBook(const DBRow& row) {
    Book b;   //create empty book
    if (row.empty()) return b;
    b.id             = row.count("id")              ? std::stoi(row.at("id"))              : 0;  //Convert string → integer
    b.title          = row.count("title")           ? row.at("title")           : "";   //If column exists → use it Otherwise → default value
    b.author         = row.count("author")          ? row.at("author")          : "";
    b.isbn           = row.count("isbn")            ? row.at("isbn")            : "";
    b.genre          = row.count("genre")           ? row.at("genre")           : "";
    b.department     = row.count("department")      ? row.at("department")      : "";
    b.bookType       = row.count("book_type")       ? row.at("book_type")       : "physical";
    b.totalCopies    = row.count("total_copies")    ? std::stoi(row.at("total_copies"))    : 1;
    b.availableCopies= row.count("available_copies")? std::stoi(row.at("available_copies")): 1;
    b.publishedYear  = row.count("published_year")  ? std::stoi(row.at("published_year"))  : 2000;
    b.description    = row.count("description")     ? row.at("description")     : "";
    b.coverUrl       = row.count("cover_url")       ? row.at("cover_url")       : "";
    b.ebookUrl       = row.count("ebook_url")       ? row.at("ebook_url")       : "";
    b.audioUrl       = row.count("audio_url")       ? row.at("audio_url")       : "";
    b.ebookUrlsJson  = row.count("ebook_urls")      ? row.at("ebook_urls")      : "[]";
    b.audioUrlsJson  = row.count("audio_urls")      ? row.at("audio_urls")      : "[]";
    if (b.ebookUrlsJson.empty()) b.ebookUrlsJson = "[]";
    if (b.audioUrlsJson.empty()) b.audioUrlsJson = "[]";
    b.borrowCount    = row.count("borrow_count")    ? std::stoi(row.at("borrow_count"))    : 0;
    b.bookCondition  = row.count("book_condition")  ? row.at("book_condition")  : "good";
    return b;
}

// ============================================================
// HELPER: Convert a Book struct to a Crow JSON object
// ============================================================
inline crow::json::wvalue bookToJson(const Book& b) {
    crow::json::wvalue j;   //Create JSON object
    j["id"]             = b.id;
    j["title"]          = b.title;      //assign values
    j["author"]         = b.author;
    j["isbn"]           = b.isbn;
    j["genre"]          = b.genre;
    j["department"]     = b.department;
    j["bookType"]       = b.bookType;
    j["totalCopies"]    = b.totalCopies;
    j["availableCopies"]= b.availableCopies;
    j["publishedYear"]  = b.publishedYear;
    j["description"]    = b.description;
    j["coverUrl"]       = b.coverUrl;
    crow::json::wvalue eList = wvalueMediaListFromStored(b.ebookUrlsJson, b.ebookUrl);
    crow::json::wvalue aList = wvalueMediaListFromStored(b.audioUrlsJson, b.audioUrl);  //convert stored JSON string → JSON object
    j["ebookUrls"]      = std::move(eList);
    j["audioUrls"]      = std::move(aList);
    {
        std::string eu = firstUrlFromJsonArray(b.ebookUrlsJson);
        if (eu.empty()) eu = b.ebookUrl;
        std::string au = firstUrlFromJsonArray(b.audioUrlsJson);
        if (au.empty()) au = b.audioUrl;
        j["ebookUrl"] = eu;
        j["audioUrl"] = au;
    }
    j["borrowCount"]    = b.borrowCount;
    j["bookCondition"]  = b.bookCondition;
    j["available"]      = b.isAvailable();  // add bool for frontend
    return j;
}

inline void registerBookRoutes(crow::SimpleApp& app)  //Function that registers all routes
 {

    // Uses: Decorator Pattern for filters, Strategy Pattern for sort
    
    CROW_ROUTE(app, "/api/books").methods(crow::HTTPMethod::Get)  //Get rout
    ([](const crow::request& req) {
        // get value from the URL
        std::string keyword   = req.url_params.get("keyword")   ? req.url_params.get("keyword")   : "";
        std::string genre     = req.url_params.get("genre")     ? req.url_params.get("genre")     : "";
        std::string type      = req.url_params.get("type")      ? req.url_params.get("type")      : "";
        std::string dept      = req.url_params.get("dept")      ? req.url_params.get("dept")      : "";
        std::string sortBy    = req.url_params.get("sort")      ? req.url_params.get("sort")      : "title";
        bool availableOnly    = std::string(req.url_params.get("available") ? req.url_params.get("available") : "") == "1";

        // Fetch all books from the database
        Database* db = Database::getInstance();
        DBResult rows = db->query("SELECT * FROM books");

        // Convert all DB rows into Book structs
        BookList allBooks;
        for (auto& row : rows) allBooks.push_back(rowToBook(row));

        //DECORATOR PATTERN: Build filter
        auto filtered = SearchFilterBuilder()
            .withKeyword(keyword)       // Add keyword filter (if keyword provided)
            .withGenre(genre)           // Add genre filter (if genre provided)
            .withType(type)             // Add type filter (physical/ebook/audio)
            .withDepartment(dept)       // Add department filter
            .build();                   // Build the complete filter chain

        if (availableOnly) {
            // If wants only available books, add availability filter too
            filtered = SearchFilterBuilder()
                .withKeyword(keyword)
                .withGenre(genre)
                .withType(type)
                .withDepartment(dept)
                .onlyAvailable()        // Adds AvailableFilter to the chain
                .build();
        }

        BookList results = filtered->filter(allBooks); // Apply the Decorator chain

        // STRATEGY PATTERN: Sort the results based on sort
        BookSorter sorter;             // Create the Context object
        sorter.setStrategy(sortBy);    // Install the correct sorting strategy
        sorter.sort(results);          // Apply the strategy to sort in-place

        // Build JSON array response
        crow::json::wvalue resp;
        resp["total"] = (int)results.size(); // Total number of books
        std::vector<crow::json::wvalue> booksJson;
        for (auto& b : results) booksJson.push_back(bookToJson(b));
        resp["books"] = std::move(booksJson);

        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    
    // route2 GET /api/books/:id — Get one book by its ID
    
    CROW_ROUTE(app, "/api/books/<int>").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req, int id) {
        Database* db = Database::getInstance();
        DBRow row = db->queryOne("SELECT * FROM books WHERE id=" + std::to_string(id));  //select 1 book
        if (row.empty()) return crow::response(404, "{\"error\":\"Book not found\"}");
        crow::response res(200, bookToJson(rowToBook(row)).dump());
        res.add_header("Content-Type", "application/json");
        return res;
    });

    
    // ROUTE 3 POST /api/books — Add a new book (librarian/admin only)
    
    CROW_ROUTE(app, "/api/books").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        // Check session and role
        SessionData session = getSession(req);
        if (session.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");  //not login
        if (session.role != "librarian" && session.role != "admin")
            return crow::response(403, "{\"error\":\"Librarian or Admin access required\"}");

        auto body = crow::json::load(req.body);   //parse JSON
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        std::string title  = body["title"].s();
        std::string author = body["author"].s();
        std::string isbn   = body["isbn"].s();
        std::string genre  = body["genre"].s();
        std::string dept   = body["department"].s();
        std::string type   = body["bookType"].s();
        int copies         = body["totalCopies"].i();
        int year           = body["publishedYear"].i();
        std::string desc   = body["description"].s();

        if (title.empty() || author.empty() || isbn.empty()) {
            return crow::response(400, "{\"error\":\"Title, author and ISBN required\"}");
        }

        Database* db = Database::getInstance();
        // Check for duplicate ISBN
        if (!db->queryOne("SELECT id FROM books WHERE isbn='" + db->escape(isbn) + "'").empty()) {
            return crow::response(409, "{\"error\":\"ISBN already exists\"}");
        }

        std::string ej, ejU, aj, ajU;
        resolveMediaFieldsFromBody(body, "ebookUrls", "ebookUrl", ej, ejU);
        resolveMediaFieldsFromBody(body, "audioUrls", "audioUrl", aj, ajU);
           //Prevent SQL injection
        bool ok = db->execute(
            "INSERT INTO books (title,author,isbn,genre,department,book_type,"
            "total_copies,available_copies,published_year,description,ebook_url,audio_url,ebook_urls,audio_urls) VALUES ('"
            + db->escape(title)  + "','" + db->escape(author) + "','"
            + db->escape(isbn)   + "','" + db->escape(genre)  + "','"
            + db->escape(dept)   + "','" + db->escape(type)   + "',"
            + std::to_string(copies) + "," + std::to_string(copies) + ","
            + std::to_string(year) + ",'" + db->escape(desc) + "','"
            + db->escape(ejU) + "','" + db->escape(ajU) + "','"
            + db->escape(ej) + "','" + db->escape(aj) + "')"
        );
        if (!ok) return crow::response(500, "{\"error\":\"Failed to add book\"}");

        crow::json::wvalue resp;
        resp["success"] = true;
        resp["id"]      = db->lastInsertId();
        resp["message"] = "Book \"" + title + "\" added successfully.";
        return crow::response(201, resp.dump());
    });


    //ROUTE 4 PUT /api/books/:id — Edit books
    CROW_ROUTE(app, "/api/books/<int>").methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, int id) {
        SessionData session = getSession(req);
        if (session.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");
        if (session.role != "librarian" && session.role != "admin")
            return crow::response(403, "{\"error\":\"Access denied\"}");

        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        Database* db = Database::getInstance();
        std::string title  = body["title"].s();
        std::string author = body["author"].s();
        std::string genre  = body["genre"].s();
        int copies         = body["totalCopies"].i();
        int available      = body["availableCopies"].i();
        std::string desc   = body["description"].s();

        std::string sql = "UPDATE books SET title='" + db->escape(title)
            + "', author='" + db->escape(author)
            + "', genre='"  + db->escape(genre)
            + "', total_copies=" + std::to_string(copies)
            + ", available_copies=" + std::to_string(available)
            + ", description='" + db->escape(desc) + "'";
        if (body.has("isbn"))
            sql += ", isbn='" + db->escape(body["isbn"].s()) + "'";
        if (body.has("bookType"))
            sql += ", book_type='" + db->escape(body["bookType"].s()) + "'";
        if (body.has("department"))
            sql += ", department='" + db->escape(body["department"].s()) + "'";
        if (body.has("publishedYear"))
            sql += ", published_year=" + std::to_string(body["publishedYear"].i());
        if (body.has("ebookUrls") || body.has("ebookUrl")) {
            std::string ej, ejU;
            resolveMediaFieldsFromBody(body, "ebookUrls", "ebookUrl", ej, ejU);
            sql += ", ebook_urls='" + db->escape(ej) + "', ebook_url='" + db->escape(ejU) + "'";
        }
        if (body.has("audioUrls") || body.has("audioUrl")) {
            std::string aj, ajU;
            resolveMediaFieldsFromBody(body, "audioUrls", "audioUrl", aj, ajU);
            sql += ", audio_urls='" + db->escape(aj) + "', audio_url='" + db->escape(ajU) + "'";
        }
        sql += " WHERE id=" + std::to_string(id);

        bool ok = db->execute(sql);
        
        // If the update was successful and copies are now available, fire the Observer to notify the waitlist!
        if (ok && available > 0) {
            WaitlistNotifier::notifyWaitlist(id, title, db);
        }

        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["message"] = ok ? "Book updated." : "Update failed.";
        return crow::response(ok ? 200 : 500, resp.dump());
    });

    
    //ROUTE5 DELETE /api/books/:id — Remove a book 
    CROW_ROUTE(app, "/api/books/<int>").methods(crow::HTTPMethod::Delete)
    ([](const crow::request& req, int id) {
        SessionData session = getSession(req);
        if (session.userId == 0) return crow::response(401, "{\"error\":\"Login required\"}");
        if (session.role != "librarian" && session.role != "admin")
            return crow::response(403, "{\"error\":\"Access denied\"}");

        Database* db = Database::getInstance();
        // cannot delete a book that's currently borrowed
        DBRow active = db->queryOne(
            "SELECT COUNT(*) as cnt FROM transactions WHERE book_id="
            + std::to_string(id) + " AND status='active'"
        );
        if (!active.empty() && std::stoi(active["cnt"]) > 0)
            return crow::response(409, "{\"error\":\"Cannot delete: book has active borrows\"}");

        bool ok = db->execute("DELETE FROM books WHERE id=" + std::to_string(id));
        crow::json::wvalue resp;
        resp["success"] = ok;
        resp["message"] = ok ? "Book deleted." : "Delete failed.";
        return crow::response(ok ? 200 : 500, resp.dump());
    });

    //Route 6 Upload PDF (ebook) or (audio)

    CROW_ROUTE(app, "/api/books/upload-media").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionData session = getSession(req);
        if (session.userId == 0)
            return crow::response(401, "{\"error\":\"Login required\"}");
        if (session.role != "librarian" && session.role != "admin")
            return crow::response(403, "{\"error\":\"Librarian or Admin access required\"}");

        std::string ct = req.get_header_value("Content-Type");
        if (ct.find("multipart/form-data") == std::string::npos)  //File upload format
            return crow::response(400, "{\"error\":\"Expected multipart/form-data\"}");

        crow::multipart::message msg(req);
        crow::multipart::part filepart = msg.get_part_by_name("file");  //get uploaded file
        if (filepart.body.empty())
            return crow::response(400, "{\"error\":\"No file field\"}");

        std::string kind = "ebook";
        crow::multipart::part kindpart = msg.get_part_by_name("category");
        if (!kindpart.body.empty())
            kind = trimStr(kindpart.body);
        if (kind != "ebook" && kind != "audio")
            return crow::response(400, "{\"error\":\"category must be ebook or audio\"}");

        std::string orig = multipartFilename(filepart);
        std::string ext  = fileExtension(orig);  //get extension
        std::string subdir;
        if (kind == "ebook") {
            if (ext != ".pdf")  //valid file 
                return crow::response(400, "{\"error\":\"E-books must be PDF (.pdf)\"}");
            subdir = "ebooks";
        } else {
            static const std::set<std::string> audioExt = {
                ".mp3", ".wav", ".ogg", ".m4a", ".webm", ".aac", ".mp4", ".mpeg"
            };
            if (!audioExt.count(ext))
                return crow::response(400, "{\"error\":\"Unsupported audio type (use mp3, wav, ogg, m4a, webm, aac, mp4)\"}");
            subdir = "audio";
        }

        std::time_t t = std::time(nullptr);
        std::string safeName = std::to_string(t) + "_" + std::to_string(session.userId) + ext;

        namespace fs = std::filesystem;
        fs::path dirPath = fs::path(uploadsBaseDir()) / "uploads" / subdir;
        std::error_code ec;
        fs::create_directories(dirPath, ec);
        fs::path fullPath = dirPath / safeName;

        std::ofstream out(fullPath.string(), std::ios::binary);  //save file
        if (!out)
            return crow::response(500, "{\"error\":\"Could not save file\"}");
        out.write(filepart.body.data(), static_cast<std::streamsize>(filepart.body.size()));  //write file content
        out.close();

        std::string publicUrl = "/uploads/" + subdir + "/" + safeName;
        crow::json::wvalue resp;
        resp["success"] = true;
        resp["url"]     = publicUrl;
        resp["message"] = "Uploaded.";
        crow::response res(200, resp.dump());
        res.add_header("Content-Type", "application/json");
        return res;   //Return file URL
    });
}
