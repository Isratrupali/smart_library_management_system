-- ============================================================
-- FILE: database/seed.sql
-- PURPOSE: Inserts demo/test data into the database.
--          Run AFTER schema.sql. The server runs this on startup
--          only if the tables are empty (avoids duplicate inserts).
-- ============================================================

-- ============================================================
-- MEMBERSHIP TIERS — Define the three tiers of membership
-- ============================================================
INSERT OR IGNORE INTO memberships (tier_name, max_books, borrow_days, fine_per_day, monthly_fee) VALUES
    -- Basic tier: free, limited to 3 books, 14-day loans
    ('basic',   3, 14, 5.0,  0.0),
    -- Premium tier: paid, up to 5 books, 21-day loans, lower fine
    ('premium', 5, 21, 3.0, 99.0),
    -- Gold tier: highest tier, up to 10 books, 30-day loans, no fine
    ('gold',   10, 30, 0.0, 199.0);

-- ============================================================
-- USERS — Three roles: admin, librarian, student
-- All passwords are "password123" for demo purposes.
-- In production, passwords should be hashed with bcrypt/SHA256.
-- ============================================================
INSERT OR IGNORE INTO users (id, name, email, password, role, membership, fine_balance) VALUES
    -- Admin user: full system access
    (1, 'Admin User',      'admin@library.edu',   'password123', 'admin',     'gold',    0.0),
    -- Librarian 1: manages books and students
    (2, 'Sarah Johnson',   'sarah@library.edu',   'password123', 'librarian', 'gold',    0.0),
    -- Librarian 2: second librarian for the system
    (3, 'Mike Williams',   'mike@library.edu',    'password123', 'librarian', 'premium', 0.0),
    -- Students: regular users who borrow books
    (4, 'Alice Brown',     'alice@student.edu',   'password123', 'student',   'basic',   0.0),
    (5, 'Bob Davis',       'bob@student.edu',     'password123', 'student',   'basic',  15.0), -- Has a fine
    (6, 'Carol White',     'carol@student.edu',   'password123', 'student',   'premium', 0.0),
    (7, 'David Lee',       'david@student.edu',   'password123', 'student',   'basic',   5.0), -- Has a fine
    (8, 'Emma Taylor',     'emma@student.edu',    'password123', 'student',   'gold',    0.0),
    (9, 'Frank Miller',    'frank@student.edu',   'password123', 'student',   'basic',   0.0),
    (10,'Grace Wilson',    'grace@student.edu',   'password123', 'student',   'premium', 0.0);

-- ============================================================
-- BOOKS — A diverse catalog with physical, ebook, and audio types
-- ============================================================
INSERT OR IGNORE INTO books (id, title, author, isbn, genre, department, book_type, total_copies, available_copies, published_year, description, borrow_count) VALUES
    -- CSE Computer Science Books
    (1,  'Clean Code',                     'Robert C. Martin',   '978-0132350884', 'Computer Science', 'CSE', 'physical', 5, 3, 2008, 'A guide to writing clean, maintainable code with best practices.', 42),
    (2,  'Design Patterns',                'Gang of Four',       '978-0201633610', 'Computer Science', 'CSE', 'physical', 4, 2, 1994, 'The classic GoF book on software design patterns.', 38),
    (3,  'Introduction to Algorithms',     'Cormen et al.',      '978-0262033848', 'Computer Science', 'CSE', 'physical', 6, 4, 2009, 'Comprehensive textbook on algorithms and data structures.', 55),
    (4,  'The Pragmatic Programmer',       'Andrew Hunt',        '978-0201616224', 'Computer Science', 'CSE', 'ebook',    3, 3, 2019, 'Career wisdom and practical tips for software developers.', 29),
    (5,  'Computer Networks',              'Andrew Tanenbaum',   '978-0132126953', 'Computer Science', 'CSE', 'physical', 5, 1, 2010, 'Comprehensive guide to computer networking fundamentals.', 47),
    (6,  'Database System Concepts',       'Abraham Silberschatz','978-0078022159', 'Computer Science', 'CSE', 'physical', 4, 3, 2019, 'Authoritative textbook on database design and management.', 33),
    (7,  'Operating System Concepts',      'Silberschatz & Galvin','978-1119800361','Computer Science', 'CSE', 'ebook',    5, 5, 2021, 'The Dinosaur Book — standard OS textbook worldwide.', 40),
    (8,  'Artificial Intelligence',        'Russell & Norvig',   '978-0134610993', 'Computer Science', 'CSE', 'audio',    3, 1, 2020, 'Standard text in artificial intelligence for CS students.', 31),
    (9,  'Python Crash Course',            'Eric Matthes',       '978-1593279288', 'Computer Science', 'CSE', 'physical', 5, 4, 2019, 'Hands-on introduction to Python programming language.', 45),
    (10, 'The C++ Programming Language',   'Bjarne Stroustrup',  '978-0321958327', 'Computer Science', 'CSE', 'ebook',    4, 4, 2013, 'Definitive reference for C++ from its creator.', 30),
    (11, 'Software Engineering',           'Ian Sommerville',    '978-0133943030', 'Computer Science', 'CSE', 'physical', 5, 3, 2015, 'Comprehensive software engineering guide for students.', 38),
    -- Mathematics Books
    (12, 'Discrete Mathematics',           'Kenneth H. Rosen',   '978-0073383095', 'Mathematics',      'Math','physical', 6, 4, 2018, 'Discrete mathematics for computer science students.', 48),
    (13, 'Calculus: Early Transcendentals','James Stewart',      '978-1285741550', 'Mathematics',      'Math','physical', 8, 5, 2015, 'The leading calculus textbook used worldwide.', 62),
    (14, 'Linear Algebra',                 'Gilbert Strang',     '978-0030105678', 'Mathematics',      'Math','ebook',    4, 3, 2016, 'Clear and intuitive exposition of linear algebra.', 27),
    -- Physics Books
    (15, 'University Physics',             'Hugh Young',         '978-0135159552', 'Physics',          'Physics','physical',5, 4, 2019,'Complete university physics with modern applications.', 35),
    (16, 'Introduction to Electrodynamics','David Griffiths',    '978-1108420419', 'Physics',          'Physics','audio',  3, 2, 2017,'Standard graduate text in classical electrodynamics.', 22),
    -- Literature Books
    (17, 'To Kill a Mockingbird',          'Harper Lee',         '978-0061935466', 'Literature',       'General','physical',4,4,1960,'Pulitzer Prize-winning novel about justice and race.', 18),
    (18, '1984',                           'George Orwell',      '978-0451524935', 'Literature',       'General','ebook',  5, 3, 1949,'Classic dystopian novel about totalitarianism.', 25),
    (19, 'The Great Gatsby',               'F. Scott Fitzgerald','978-0743273565', 'Literature',       'General','audio',  4, 4, 1925,'Story of wealth and the American Dream in the 1920s.', 14),
    -- Self-Help / Business
    (20, 'The 7 Habits',                   'Stephen Covey',      '978-1982137274', 'Self-Help',        'General','audio',  3, 2, 1989,'Seven habits of highly effective people.', 16);

-- ============================================================
-- TRANSACTIONS — Some active borrows and a returned one
-- ============================================================
INSERT OR IGNORE INTO transactions (id, user_id, book_id, borrow_date, due_date, return_date, fine, status) VALUES
    -- Alice has 2 active borrows (within her basic limit of 3)
    (1, 4, 1,  '2026-04-20', '2026-05-04', NULL,         0.0,  'active'),   -- Clean Code, due May 4
    (2, 4, 3,  '2026-04-22', '2026-05-06', NULL,         0.0,  'active'),   -- Algorithms, due May 6
    -- Bob has an overdue book (fine 15 BDT)
    (3, 5, 5,  '2026-04-05', '2026-04-19', NULL,         15.0, 'overdue'),  -- Computer Networks, overdue
    -- Carol has an active borrow
    (4, 6, 2,  '2026-04-18', '2026-05-09', NULL,         0.0,  'active'),   -- Design Patterns (21-day premium)
    -- David has a small fine from a returned book
    (5, 7, 9,  '2026-04-01', '2026-04-15', '2026-04-20', 5.0, 'returned'), -- Python Crash Course, returned late
    -- Emma has a current borrow
    (6, 8, 7,  '2026-04-25', '2026-05-25', NULL,         0.0,  'active');   -- OS Concepts (gold 30-day)

-- ============================================================
-- NOTIFICATIONS — Some sample notification messages
-- ============================================================
INSERT OR IGNORE INTO notifications (user_id, message, is_read) VALUES
    -- Bob: told about his overdue book fine
    (5, 'Your book "Computer Networks" is overdue! Fine of 15 BDT has been added to your account.', 0),
    -- Alice: welcome message on registration
    (4, 'Welcome to SmartLibrary, Alice! You can borrow up to 3 books with your Basic membership.', 1),
    -- Grace: informed her seat booking was confirmed
    (10, 'Your seat booking for Reading Room A (Slot: 09:00-11:00) has been confirmed!', 0);

-- ============================================================
-- SEAT BOOKINGS — Some sample reading room reservations
-- ============================================================
INSERT OR IGNORE INTO seat_bookings (user_id, seat_number, room, booking_date, time_slot, status) VALUES
    -- Alice booked seat A1 for tomorrow
    (4,  'A1', 'Reading Room A', '2026-05-03', '09:00-11:00', 'confirmed'),
    -- Emma booked seat B2 for this afternoon
    (8,  'B2', 'Reading Room B', '2026-05-02', '15:00-17:00', 'confirmed'),
    -- Grace booked seat A3
    (10, 'A3', 'Reading Room A', '2026-05-03', '11:00-13:00', 'confirmed');
