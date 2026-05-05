// DESIGN PATTERN: FACTORY METHOD

#pragma once                        

#include "../models/User.h"        
#include <string>                  
#include <memory>                 
#include <stdexcept>            

// CLASS: UserFactory- Creates the correct User subclass by role

class UserFactory {
public:
    static std::unique_ptr<IUser> create(const std::string& role) {
        if (role == "student") {
            return std::make_unique<StudentUser>(); 
        }

        else if (role == "librarian") {
            return std::make_unique<LibrarianUser>();
        }

        else if (role == "admin") {
            return std::make_unique<AdminUser>();    
        }
        else {
            throw std::invalid_argument(
                "[UserFactory] Unknown role: '" + role + "'. "
                "Expected 'student', 'librarian', or 'admin'."
            );
        }
    }

    static bool isValidRole(const std::string& role) {
        // Return true only for the three allowed role strings
        return (role == "student" || role == "librarian" || role == "admin");
    }

    
    static int getMaxBooksForRole(const std::string& role) {
        auto user = create(role);   // Create the user object using the Factory
        return user->getMaxBooks(); // Ask it for the max books limit
    }

    //  get borrow days for a role.

    static int getBorrowDaysForRole(const std::string& role) {
        auto user = create(role);       // Factory creates the correct subclass
        return user->getBorrowDays();   // Get the borrow duration for this role
    }
};
