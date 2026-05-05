
-- FILE: database/schema.sql
-- TABLE: users

CREATE TABLE IF NOT EXISTS users (
    id              INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique ID; auto-increments with each new user
    name            TEXT    NOT NULL,                  -- Full name of the user (e.g., "Alice Brown")
    email           TEXT    NOT NULL UNIQUE,           -- Login email; UNIQUE ensures no duplicate accounts
    password        TEXT    NOT NULL,                  -- Password (plain text for demo; hash in production)
    role            TEXT    NOT NULL DEFAULT 'student',-- Role: 'student', 'librarian', or 'admin'
    membership      TEXT    NOT NULL DEFAULT 'basic',  -- Membership tier: 'basic', 'premium', 'gold'
    fine_balance    REAL    NOT NULL DEFAULT 0.0,      -- Outstanding fine in BDT; 0 means no debt
    is_suspended    INTEGER NOT NULL DEFAULT 0,        -- 0 = active account, 1 = suspended by admin
    created_at      TEXT    NOT NULL DEFAULT (datetime('now')) -- Timestamp when account was created
);

-- TABLE: books

CREATE TABLE IF NOT EXISTS books (
    id              INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique book ID
    title           TEXT    NOT NULL,                  -- Book title (e.g., "Clean Code")
    author          TEXT    NOT NULL,                  -- Author name (e.g., "Robert C. Martin")
    isbn            TEXT    NOT NULL UNIQUE,           -- International Standard Book Number; must be unique
    genre           TEXT    NOT NULL,                  -- Genre category (e.g., "Computer Science", "Fiction")
    department      TEXT    NOT NULL DEFAULT '',       -- Academic department (e.g., "CSE", "Math")
    book_type       TEXT    NOT NULL DEFAULT 'physical',-- Type: 'physical', 'ebook', or 'audio'
    total_copies    INTEGER NOT NULL DEFAULT 1,        -- Total physical copies owned by the library
    available_copies INTEGER NOT NULL DEFAULT 1,       -- Copies currently available to borrow
    published_year  INTEGER NOT NULL DEFAULT 2000,     -- Year of publication
    description     TEXT    NOT NULL DEFAULT '',       -- Short summary or description of the book
    cover_url       TEXT    NOT NULL DEFAULT '',       -- URL to cover image (for UI display)
    ebook_url       TEXT    NOT NULL DEFAULT '',       -- First / primary ebook file URL (legacy + convenience)
    audio_url       TEXT    NOT NULL DEFAULT '',       -- First / primary audio file URL
    ebook_urls      TEXT    NOT NULL DEFAULT '[]',     -- JSON array: [{"url":"","label":"Part 1"}, ...]
    audio_urls      TEXT    NOT NULL DEFAULT '[]',     -- JSON array for multi-part audiobooks
    borrow_count    INTEGER NOT NULL DEFAULT 0,        -- How many times borrowed (used for popularity sort)
    book_condition  TEXT    NOT NULL DEFAULT 'good',   -- Physical condition: 'excellent','good','fair','poor'
    created_at      TEXT    NOT NULL DEFAULT (datetime('now')) -- When this book was added to the catalog
);


-- TABLE: transactions
-- PURPOSE: Records every borrow and return event.
-- The Facade Pattern manages the borrow/return workflow.

CREATE TABLE IF NOT EXISTS transactions (
    id              INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique transaction ID
    user_id         INTEGER NOT NULL,                  -- Which user borrowed/returned the book
    book_id         INTEGER NOT NULL,                  -- Which book was borrowed/returned
    borrow_date     TEXT    NOT NULL DEFAULT (datetime('now')), -- When the book was borrowed
    due_date        TEXT    NOT NULL,                  -- When the book must be returned
    return_date     TEXT    NULL DEFAULT NULL,         -- Actual return date; NULL if not yet returned
    fine            REAL    NOT NULL DEFAULT 0.0,      -- Fine charged on this transaction (overdue fee)
    status          TEXT    NOT NULL DEFAULT 'active', -- 'active', 'returned', or 'overdue'
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, -- Delete transaction if user deleted
    FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE  -- Delete transaction if book deleted
);


-- TABLE: seat_bookings
-- PURPOSE: Students can reserve a reading room seat for a time slot.

CREATE TABLE IF NOT EXISTS seat_bookings (
    id              INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique seat booking ID
    user_id         INTEGER NOT NULL,                  -- Which student booked the seat
    seat_number     TEXT    NOT NULL,                  -- Seat identifier (e.g., "A1", "B3")
    room            TEXT    NOT NULL DEFAULT 'Reading Room A', -- Which room the seat is in
    booking_date    TEXT    NOT NULL,                  -- Date of the booking (YYYY-MM-DD)
    time_slot       TEXT    NOT NULL,                  -- Time slot (e.g., "09:00-11:00")
    status          TEXT    NOT NULL DEFAULT 'confirmed', -- 'confirmed', 'cancelled', 'completed'
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE -- Remove booking if user deleted
);


-- TABLE: notifications
-- PURPOSE: Stores messages sent to users (e.g., "Your book is available!").
-- DESIGN PATTERN: Observer Pattern creates these notification rows
--                 when a waitlisted book becomes available.

CREATE TABLE IF NOT EXISTS notifications (
    id              INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique notification ID
    user_id         INTEGER NOT NULL,                  -- Which user receives this notification
    message         TEXT    NOT NULL,                  -- The notification message text
    is_read         INTEGER NOT NULL DEFAULT 0,        -- 0 = unread, 1 = read by the user
    created_at      TEXT    NOT NULL DEFAULT (datetime('now')), -- When the notification was sent
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE -- Remove notification if user deleted
);


-- TABLE: waitlist
-- PURPOSE: Queue of users waiting for an unavailable book.
--          When book is returned, Observer Pattern notifies them.

CREATE TABLE IF NOT EXISTS waitlist (
    id              INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique waitlist entry ID
    user_id         INTEGER NOT NULL,                  -- Which user is waiting
    book_id         INTEGER NOT NULL,                  -- Which book they're waiting for
    position        INTEGER NOT NULL DEFAULT 1,        -- Their queue position (1 = first)
    status          TEXT    NOT NULL DEFAULT 'waiting',-- 'waiting', 'notified', or 'fulfilled'
    joined_at       TEXT    NOT NULL DEFAULT (datetime('now')), -- When they joined the waitlist
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (book_id) REFERENCES books(id)  ON DELETE CASCADE
);


-- TABLE: book_requests
-- PURPOSE: Students can request books not yet in the catalog.
--          Librarians/admins review and approve/reject these.

CREATE TABLE IF NOT EXISTS book_requests (
    id              INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique request ID
    user_id         INTEGER NOT NULL,                  -- Which student made the request
    book_title      TEXT    NOT NULL,                  -- Title of the requested book
    author          TEXT    NOT NULL DEFAULT '',       -- Author of the requested book
    reason          TEXT    NOT NULL DEFAULT '',       -- Why the student wants this book
    status          TEXT    NOT NULL DEFAULT 'pending',-- 'pending', 'approved', 'rejected'
    admin_note      TEXT    NOT NULL DEFAULT '',       -- Response note from librarian/admin
    requested_at    TEXT    NOT NULL DEFAULT (datetime('now')), -- When the request was made
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);


-- TABLE: memberships
-- PURPOSE: Defines the different membership tiers and their benefits.
-- ADMIN can change these settings via the Admin panel.

CREATE TABLE IF NOT EXISTS memberships (
    id              INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique membership tier ID
    tier_name       TEXT    NOT NULL UNIQUE,           -- Tier name: 'basic', 'premium', 'gold'
    max_books       INTEGER NOT NULL DEFAULT 3,        -- Max books a user at this tier can borrow
    borrow_days     INTEGER NOT NULL DEFAULT 14,       -- How many days they can keep a book
    fine_per_day    REAL    NOT NULL DEFAULT 5.0,      -- Overdue fine per day in BDT
    monthly_fee     REAL    NOT NULL DEFAULT 0.0       -- Monthly fee in BDT (0 = free)
);


-- TABLE: membership_bills
-- PURPOSE: Billing for membership upgrades. Students "purchase" a tier;
--          amount due is recorded; librarian/admin marks paid and tier applies.

CREATE TABLE IF NOT EXISTS membership_bills (
    id                    INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id               INTEGER NOT NULL,
    target_tier           TEXT    NOT NULL,
    amount_bdt            REAL    NOT NULL,
    status                TEXT    NOT NULL DEFAULT 'pending',
    created_at            TEXT    NOT NULL DEFAULT (datetime('now')),
    paid_at               TEXT    NULL DEFAULT NULL,
    recorded_by_user_id   INTEGER NULL DEFAULT NULL,
    description           TEXT    NOT NULL DEFAULT '',
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (recorded_by_user_id) REFERENCES users(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS idx_bills_user_status ON membership_bills(user_id, status);


-- INDEX: Speed up common queries on the transactions table

CREATE INDEX IF NOT EXISTS idx_tx_user   ON transactions(user_id, status); -- Fast lookup: user's active borrows
CREATE INDEX IF NOT EXISTS idx_tx_book   ON transactions(book_id, status); -- Fast lookup: who has this book
CREATE INDEX IF NOT EXISTS idx_notif_user ON notifications(user_id, is_read); -- Fast: unread notifications

-- Migration: multi-part ebook/audio (safe to re-run; may log errors if columns already exist)
ALTER TABLE books ADD COLUMN ebook_urls TEXT NOT NULL DEFAULT '[]';
ALTER TABLE books ADD COLUMN audio_urls TEXT NOT NULL DEFAULT '[]';
