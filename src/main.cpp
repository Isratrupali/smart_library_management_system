//   The entry point for the entire application.
//   1. Opens the SQLite database (Singleton Pattern)
//   2. Runs schema.sql and seed.sql to set up tables
//   3. Creates the Crow HTTP server
//   4. Registers all API routes from handler files
//   5. Sets up static file serving for the frontend
//   6. Starts listening on port 8080
//
// HOW TO BUILD:
//   Run compile.bat (Windows) — see README.md
//
// HOW TO RUN:
//   Run run.bat — then open http://localhost:8080 in your browser
//
// DESIGN PATTERNS IN THIS FILE:
//   - Singleton: Database::getInstance() — one DB connection
//   - Factory, Strategy, Decorator, Observer, Facade: used inside handlers
// ============================================================

// ---- Crow HTTP Framework ----
// ASIO_STANDALONE: use standalone ASIO (no Boost dependency).
// Do NOT define CROW_ENABLE_SSL — leaving it undefined disables SSL/OpenSSL.
#define ASIO_STANDALONE
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "crow_all.h"

// ---- All handler files ----
// Each handler file registers a group of related API routes.
// Including them here makes their registerXxxRoutes() functions available.
#include "handlers/AuthHandler.h"       // Login, register, logout
#include "handlers/BookHandler.h"       // Book catalog CRUD + search/filter/sort
#include "handlers/StudentHandler.h"    // Student borrows, returns, seats, notifications
#include "handlers/LibrarianHandler.h"  // Librarian management routes
#include "handlers/AdminHandler.h"      // Admin control panel routes

// ---- Standard library ----
#include <iostream>     // std::cout, std::cerr — for startup messages
#include <string>       // std::string
#include <fstream>      // std::ifstream — for reading static frontend files
#include <filesystem>   // create_directories — uploads folders next to slms.exe
#include <cctype>       // std::tolower — MIME from extension
#ifdef _WIN32
#include <windows.h>    // GetModuleFileNameA — resolve files next to slms.exe
#endif

// ============================================================
// Directory containing slms.exe (so static files work even if cwd differs)
// ============================================================
inline std::string slmsExecutableDir() {
#ifdef _WIN32
    char buf[MAX_PATH] = {0};
    if (GetModuleFileNameA(nullptr, buf, MAX_PATH)) {
        std::string full(buf);
        std::size_t pos = full.find_last_of("\\/");
        if (pos != std::string::npos) return full.substr(0, pos);
    }
#endif
    return ".";
}

// ============================================================
// HELPER: Serve a static HTML/CSS/JS file from the frontend/ folder
// ============================================================
// Tries: (1) path as given, (2) next to slms.exe — fixes 404 when cwd is wrong.
// ============================================================
inline crow::response serveStaticFile(const std::string& filepath) {
    std::string content;
    std::string openedFrom;
    auto tryOpen = [&](const std::string& path) -> bool {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        content.assign((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
        openedFrom = path;
        return true;
    };

    if (!tryOpen(filepath)) {
        const std::string base = slmsExecutableDir();
        if (!tryOpen(base + "/" + filepath) && !tryOpen(base + "\\" + filepath)) {
            return crow::response(404,
                "<h1>404 — Page Not Found</h1><p>Could not read: " + filepath
                + "</p><p>Run the server via <strong>run.bat</strong> from the project folder, "
                "or rebuild with <strong>compile.bat</strong> after adding new pages.</p>");
        }
    }

    std::string contentType = "text/html";
    if (openedFrom.find(".css") != std::string::npos)  contentType = "text/css";
    if (openedFrom.find(".js")  != std::string::npos)  contentType = "application/javascript";
    if (openedFrom.find(".png") != std::string::npos)  contentType = "image/png";
    if (openedFrom.find(".jpg") != std::string::npos)  contentType = "image/jpeg";
    if (openedFrom.find(".ico") != std::string::npos)  contentType = "image/x-icon";

    crow::response res(200, content);
    res.add_header("Content-Type", contentType);
    return res;
}

// ============================================================
// HELPER: Serve uploaded PDF / audio from uploads/ebooks|audio/
// ============================================================
inline bool isSafeUploadSegment(const std::string& n) {
    if (n.empty() || n.size() > 255) return false;
    if (n.find("..") != std::string::npos) return false;
    for (unsigned char c : n) {
        if (std::isalnum(c) || c == '.' || c == '_' || c == '-') continue;
        return false;
    }
    return true;
}

inline std::string mimeTypeForUpload(const std::string& fname) {
    auto pos = fname.find_last_of('.');
    std::string ext = pos != std::string::npos ? fname.substr(pos) : "";
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".mp3") return "audio/mpeg";
    if (ext == ".wav") return "audio/wav";
    if (ext == ".ogg") return "audio/ogg";
    if (ext == ".m4a") return "audio/mp4";
    if (ext == ".aac") return "audio/aac";
    if (ext == ".webm") return "audio/webm";
    if (ext == ".mp4") return "video/mp4";
    if (ext == ".mpeg") return "audio/mpeg";
    return "application/octet-stream";
}

inline crow::response serveUploadedFile(const std::string& subdir, std::string fname) {
    if (!isSafeUploadSegment(fname))
        return crow::response(400, "Invalid filename");
    namespace fs = std::filesystem;
    fs::path full = fs::path(slmsExecutableDir()) / "uploads" / subdir / fname;
    std::ifstream file(full.string(), std::ios::binary);
    if (!file.is_open())
        return crow::response(404, "Not found");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    std::string mt = mimeTypeForUpload(fname);
    crow::response res(200, content);
    res.add_header("Content-Type", mt);
    if (mt == "application/pdf")
        res.add_header("Content-Disposition", "inline; filename=\"" + fname + "\"");
    return res;
}


// MAIN: Application Entry Point

int main() {
    std::cout << "============================================\n";
    std::cout << "  Smart Library Management System (SLMS)\n";
    std::cout << "  CSE327 — Software Engineering Project\n";
    std::cout << "============================================\n";


    // STEP 1: Initialize the Database (Singleton Pattern)

    Database* db = Database::getInstance(); // Singleton: first call creates the DB
    std::cout << "[SLMS] Database initialized.\n";


    // STEP 2: Create tables (run schema.sql)
    db->runSqlFile("database/schema.sql"); // Creates all tables defined in schema.sql
    std::cout << "[SLMS] Schema loaded.\n";

    // STEP 3: Insert demo data (run seed.sql) if tables are empty
    DBRow userCount = db->queryOne("SELECT COUNT(*) AS cnt FROM users");
    if (userCount["cnt"] == "0") {
        db->runSqlFile("database/seed.sql"); // Only seed if database is empty
        std::cout << "[SLMS] Demo data seeded.\n";
    } else {
        std::cout << "[SLMS] Database already has data — skipping seed.\n";
    }

    {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::create_directories(fs::path(slmsExecutableDir()) / "uploads" / "ebooks", ec);
        fs::create_directories(fs::path(slmsExecutableDir()) / "uploads" / "audio", ec);
    }


    // STEP 4: Create the Crow HTTP application
    crow::SimpleApp app; // Create the Crow web server instance

    // STEP 5: Register all API routes from handler files
    registerAuthRoutes(app);       // POST /api/login, /api/register, /api/logout, GET /api/me
    registerBookRoutes(app);       // GET/POST/PUT/DELETE /api/books, /api/books/:id
    registerStudentRoutes(app);    // GET/POST /api/student/*
    registerLibrarianRoutes(app);  // GET/POST /api/librarian/*
    registerAdminRoutes(app);      // GET/POST/PUT/DELETE /api/admin/*
    std::cout << "[SLMS] All API routes registered.\n";

    CROW_ROUTE(app, "/uploads/ebooks/<string>")
    ([](std::string fname) {
        return serveUploadedFile("ebooks", std::move(fname));
    });
    CROW_ROUTE(app, "/uploads/audio/<string>")
    ([](std::string fname) {
        return serveUploadedFile("audio", std::move(fname));
    });

    // STEP 6: Serve the frontend index page at the root URL "/"
    CROW_ROUTE(app, "/")([]{
        return serveStaticFile("frontend/index.html"); // Serve the landing page
    });

    // STEP 7: Serve static HTML pages (frontend routes)

    // Auth page (login/register)
    CROW_ROUTE(app, "/auth.html")([]{ return serveStaticFile("frontend/auth.html"); });

    // Student portal pages
    CROW_ROUTE(app, "/student/dashboard.html")([]{ return serveStaticFile("frontend/student/dashboard.html"); });
    CROW_ROUTE(app, "/student/catalog.html")([]{ return serveStaticFile("frontend/student/catalog.html"); });
    CROW_ROUTE(app, "/student/ebooks.html")([]{ return serveStaticFile("frontend/student/ebooks.html"); });
    CROW_ROUTE(app, "/student/membership.html")([]{ return serveStaticFile("frontend/student/membership.html"); });
    CROW_ROUTE(app, "/student/billing.html")([]{ return serveStaticFile("frontend/student/billing.html"); });
    CROW_ROUTE(app, "/student/seats.html")([]{ return serveStaticFile("frontend/student/seats.html"); });

    // Librarian portal pages
    CROW_ROUTE(app, "/librarian/dashboard.html")([]{ return serveStaticFile("frontend/librarian/dashboard.html"); });
    CROW_ROUTE(app, "/librarian/manage-books.html")([]{ return serveStaticFile("frontend/librarian/manage-books.html"); });
    CROW_ROUTE(app, "/librarian/borrowing.html")([]{ return serveStaticFile("frontend/librarian/borrowing.html"); });
    CROW_ROUTE(app, "/librarian/students.html")([]{ return serveStaticFile("frontend/librarian/students.html"); });
    CROW_ROUTE(app, "/librarian/notifications.html")([]{ return serveStaticFile("frontend/librarian/notifications.html"); });
    CROW_ROUTE(app, "/librarian/billing.html")([]{ return serveStaticFile("frontend/librarian/billing.html"); });
    CROW_ROUTE(app, "/librarian/seats.html")([]{ return serveStaticFile("frontend/librarian/seats.html"); });

    // Admin portal pages
    CROW_ROUTE(app, "/admin/dashboard.html")([]{ return serveStaticFile("frontend/admin/dashboard.html"); });
    CROW_ROUTE(app, "/admin/users.html")([]{ return serveStaticFile("frontend/admin/users.html"); });
    CROW_ROUTE(app, "/admin/membership.html")([]{ return serveStaticFile("frontend/admin/membership.html"); });
    CROW_ROUTE(app, "/admin/settings.html")([]{ return serveStaticFile("frontend/admin/settings.html"); });
    CROW_ROUTE(app, "/admin/statistics.html")([]{ return serveStaticFile("frontend/admin/statistics.html"); });
    // Same membership-billing UI as librarian (admin uses checkLibrarian + billing APIs)
    CROW_ROUTE(app, "/admin/billing.html")([]{ return serveStaticFile("frontend/librarian/billing.html"); });

    
    // STEP 8: Serve CSS and JS static assets
    
    CROW_ROUTE(app, "/css/style.css")([]{ return serveStaticFile("frontend/css/style.css"); });
    CROW_ROUTE(app, "/js/auth.js")([]{ return serveStaticFile("frontend/js/auth.js"); });
    CROW_ROUTE(app, "/js/student.js")([]{ return serveStaticFile("frontend/js/student.js"); });
    CROW_ROUTE(app, "/js/librarian.js")([]{ return serveStaticFile("frontend/js/librarian.js"); });
    CROW_ROUTE(app, "/js/admin.js")([]{ return serveStaticFile("frontend/js/admin.js"); });

     
    // STEP 9: Start the server
    
    std::cout << "\n[SLMS] Server started at http://localhost:8080\n";
    std::cout << "[SLMS] Press Ctrl+C to stop.\n\n";
    std::cout << "Demo Login Credentials:\n";
    std::cout << "  Student:   alice@student.edu  / password123\n";
    std::cout << "  Librarian: sarah@library.edu  / password123\n";
    std::cout << "  Admin:     admin@library.edu  / password123\n\n";

    app.port(8080)          // Listen on port 8080
       .multithreaded()     // Use multiple threads for concurrent requests
       .run();              // Start the server event loop

    //
    // STEP 10: Cleanup after server stops (Ctrl+C)
    
    Database::destroy(); // Close the SQLite connection gracefully
    std::cout << "[SLMS] Server stopped. Goodbye!\n";

    return 0; // Program exits cleanly
}
