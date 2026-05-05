// Data structure for notifications sent to users.
//These are created by the Observer Pattern when waitlisted book becomes available or by librarians.

#pragma once
#include <string>   

// notification message for  user.
// same as the notifications table in schema.sql.

struct Notification {
    int         id        = 0;      // notification ID
    int         userId    = 0;      // Which user this notification is for
    std::string message   = "";     // notification text shown to the user
    int         isRead    = 0;      // 0 = unread , 1 = already read
    std::string createdAt = "";     // When this notification was sent
};

// SeatBooking — A reading room seat reservation.
// 'seat_bookings' table in schema.sql.

struct SeatBooking {
    int         id          = 0;              // booking ID
    int         userId      = 0;              // which student reserved this seat
    std::string seatNumber  = "";             // Seat num
    std::string room        = "Reading Room A"; // Room name
    std::string bookingDate = "";             // Date of the reservation 
    std::string timeSlot    = "";             // Time
    std::string status      = "confirmed";    // status

    // Joined field: student's name from JOIN with users table, for display
    std::string userName    = "";             // Name of the student who booked this seat
};
