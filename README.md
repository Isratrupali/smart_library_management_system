# 📚 SmartLibrary — Smart Library Management System
**CSE327 Software Engineering Project**

---

## Technology Stack

| Layer | Technology |
|---|---|
| **Backend** | C++17 + [Crow](https://crowcpp.org/) HTTP framework |
| **Database** | SQLite3 (single-file, no server needed) |
| **Frontend** | HTML5 + Bootstrap 5 + Vanilla JavaScript |
| **Build** | g++ (MinGW-w64) via `compile.bat` |

---

## Design Patterns Implemented

| Pattern | File | How It's Used |
|---|---|---|
| **Singleton** | `src/database/Database.h` | One shared SQLite connection for the whole app |
| **Factory** | `src/patterns/UserFactory.h` | Creates Student/Librarian/Admin objects with role-specific rules |
| **Strategy** | `src/patterns/SortStrategy.h` | Swappable book sort: title, author, year, popularity |
| **Decorator** | `src/patterns/SearchDecorator.h` | Stackable search filters: keyword + genre + type + availability |
| **Observer** | `src/patterns/Observer.h` | Notifies waitlisted users when a returned book becomes available |
| **Facade** | `src/patterns/Facade.h` | Provides a simple unified interface for borrow/return workflows |

---

## Quick Start (3 Steps)

### Step 1 — Download Libraries
```bat
setup.bat
```
This downloads `crow_all.h`, `sqlite3.h`, and `sqlite3.c` into `libs/`.
> **Requires:** `curl` (built into Windows 10/11)

### Step 2 — Compile
```bat
compile.bat
```
This produces `slms.exe`. Takes ~30–90 seconds (Crow is a large header).
> **Requires:** g++ 15.x (MinGW-w64) — you already have this ✅

### Step 3 — Run
```bat
run.bat
```
Opens `http://localhost:8080` in your browser automatically.

---

## Project Structure

```
f:\CSE327 project\
├── setup.bat              ← STEP 1: Download libs
├── compile.bat            ← STEP 2: Build slms.exe
├── run.bat                ← STEP 3: Start server
├── slms.exe               ← (created after compiling)
├── slms.db                ← (created automatically on first run)
│
├── libs\                  ← Third-party headers (filled by setup.bat)
│   ├── crow_all.h
│   ├── sqlite3.h
│   └── sqlite3.c
│
├── src\
│   ├── main.cpp           ← Entry point: registers all Crow routes
│   ├── database\
│   │   └── Database.h     ← SINGLETON PATTERN
│   ├── models\
│   │   ├── User.h         ← IUser + StudentUser/LibrarianUser/AdminUser
│   │   ├── Book.h
│   │   ├── Transaction.h
│   │   └── Notification.h
│   ├── patterns\
│   │   ├── UserFactory.h      ← FACTORY PATTERN
│   │   ├── SortStrategy.h     ← STRATEGY PATTERN
│   │   ├── SearchDecorator.h  ← DECORATOR PATTERN
│   │   ├── Observer.h         ← OBSERVER PATTERN
│   │   └── Facade.h           ← FACADE PATTERN
│   └── handlers\
│       ├── AuthHandler.h      ← /api/login, /api/register
│       ├── BookHandler.h      ← /api/books CRUD
│       ├── StudentHandler.h   ← /api/student/*
│       ├── LibrarianHandler.h ← /api/librarian/*
│       └── AdminHandler.h     ← /api/admin/*
│
├── database\
│   ├── schema.sql         ← CREATE TABLE statements
│   └── seed.sql           ← Demo data (users, books, transactions)
│
└── frontend\
    ├── index.html         ← Landing page
    ├── auth.html          ← Login / Register
    ├── css\style.css      ← Dark glassmorphism theme
    ├── js\
    │   ├── auth.js
    │   ├── student.js
    │   ├── librarian.js
    │   └── admin.js
    ├── student\           ← dashboard, catalog, ebooks, membership, seats
    ├── librarian\         ← dashboard, manage-books, borrowing, students, notifications
    └── admin\             ← dashboard, users, membership, statistics, settings
```

---

## Demo Accounts

| Role | Email | Password |
|---|---|---|
| 🎓 Student | alice@student.edu | password123 |
| 📋 Librarian | sarah@library.edu | password123 |
| ⚙️ Admin | admin@library.edu | password123 |

---

## API Endpoints

### Auth
```
POST /api/login      { email, password }
POST /api/register   { name, email, password }
POST /api/logout
GET  /api/me
```

### Books (Decorator + Strategy applied here)
```
GET  /api/books?keyword=&genre=&type=&sort=title&available=1
GET  /api/books/:id
POST /api/books      (librarian/admin)
PUT  /api/books/:id  (librarian/admin)
DELETE /api/books/:id (librarian/admin)
```

### Student (Facade Pattern)
```
GET  /api/student/borrows
POST /api/student/borrow      { bookId }    ← LibraryFacade
POST /api/student/return      { transactionId } ← LibraryFacade + Observer
GET  /api/student/seats
POST /api/student/seats/book  { seatNumber, date, timeSlot, room }
GET  /api/student/notifications
POST /api/student/waitlist    { bookId }
```

### Librarian
```
GET  /api/librarian/stats
GET  /api/librarian/borrows
GET  /api/librarian/overdue
GET  /api/librarian/students
POST /api/librarian/return    { transactionId }
POST /api/librarian/notify    { userId, message }
```

### Admin
```
GET    /api/admin/stats
GET    /api/admin/users
POST   /api/admin/users       { name, email, password, role }
PUT    /api/admin/users/:id   { role, membership, isSuspended, fineBalance }
DELETE /api/admin/users/:id
GET    /api/admin/requests
PUT    /api/admin/requests/:id { status, adminNote }
```

---

## Manual Library Download (if setup.bat fails)

If `curl` isn't available, download these files manually:

1. **crow_all.h** → https://github.com/CrowCpp/Crow/releases/tag/v1.2.0
   - Download `crow_all.h` → save to `libs\crow_all.h`

2. **SQLite Amalgamation** → https://sqlite.org/download.html
   - Download `sqlite-amalgamation-*.zip`
   - Extract `sqlite3.h` and `sqlite3.c` → save to `libs\`

---

## Common Compile Errors

| Error | Fix |
|---|---|
| `crow_all.h: No such file` | Run `setup.bat` first |
| `undefined reference to WSA...` | Add `-lws2_32 -lmswsock` (already in compile.bat) |
| `error: 'optional' not in scope` | Add `-std=c++17` (already in compile.bat) |
| `ld: cannot find -lpthread` | Install `mingw-w64-x86_64-winpthreads` |

---

## Project by
**CSE327 — Software Engineering**  
Built with C++ Crow + SQLite + Bootstrap 5  
*Singleton · Factory · Strategy · Decorator · Observer · Facade*
