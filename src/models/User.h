// PURPOSE: Defines the User data structure (plain data container).
// Also includes the abstract User interface used by the Factory Pattern to enforce role-based rules.

#pragma once            

#include <string>       

struct UserData //UserData= data structure for a user record; holds data from database
{
    int         id          = 0;          
    std::string name        = "";         
    std::string email       = "";         
    std::string password    = "";        
    std::string role        = "student";  
    std::string membership  = "basic";  
    double      fineBalance = 0.0;       
    int         isSuspended = 0;          // 0 = active, 1 = account suspended
    std::string createdAt   = "";         // Account creation timestamp
};

// ABSTRACT CLASS: IUser- Interface for the Factory Pattern

class IUser {
public:
    virtual std::string getRole()           const = 0; // Returns "student", "librarian", or "admin"
    virtual std::string getDisplayRole()    const = 0; // Returns human-readable "Student", etc.
    virtual int         getMaxBooks()       const = 0; // Max books allowed to borrow at once
    virtual int         getBorrowDays()     const = 0; // How many days a borrow lasts
    virtual double      getFinePerDay()     const = 0; // Overdue fine per day in BDT
    virtual bool        canManageBooks()    const = 0; // Can add/edit/delete books?
    virtual bool        canManageUsers()    const = 0; // Can create/suspend user accounts?
    virtual bool        canSendNotifs()     const = 0; // Can send notifications to users?
    virtual bool        canViewStats()      const = 0; // Can see system analytics?
    virtual std::string getDashboardPath()  const = 0; // URL path to their dashboard page
    virtual ~IUser() = default;                        // Virtual destructor for safe polymorphism
};


// CONCRETE: StudentUser- Implements IUser for the Student role

class StudentUser : public IUser //inheritance; StudentUser class inherits from IUser class
{
public:
    std::string getRole()           const override { return "student"; }
    std::string getDisplayRole()    const override { return "Student"; }
    int         getMaxBooks()       const override { return 3; }            
    int         getBorrowDays()     const override { return 14; }      
    double      getFinePerDay()     const override { return 5.0; }          
    bool        canManageBooks()    const override { return false; }        
    bool        canManageUsers()    const override { return false; }     
    bool        canSendNotifs()     const override { return false; }     
    bool        canViewStats()      const override { return false; }       
    std::string getDashboardPath()  const override { return "/student/dashboard.html"; }
};


// CONCRETE: LibrarianUser- Implements IUser for the Librarian role

class LibrarianUser : public IUser {
public:
    std::string getRole()           const override { return "librarian"; }
    std::string getDisplayRole()    const override { return "Librarian"; }
    int         getMaxBooks()       const override { return 10; }          
    int         getBorrowDays()     const override { return 30; }           
    double      getFinePerDay()     const override { return 0.0; }      
    bool        canManageBooks()    const override { return true; }        
    bool        canManageUsers()    const override { return false; }       
    bool        canSendNotifs()     const override { return true; }        
    bool        canViewStats()      const override { return true; }        
    std::string getDashboardPath()  const override { return "/librarian/dashboard.html"; }
};

// CONCRETE: AdminUser- Implements IUser for the Admin role

class AdminUser : public IUser {
public:
    std::string getRole()           const override { return "admin"; }
    std::string getDisplayRole()    const override { return "Administrator"; }
    int         getMaxBooks()       const override { return 999; }          
    int         getBorrowDays()     const override { return 60; }         
    double      getFinePerDay()     const override { return 0.0; }          
    bool        canManageBooks()    const override { return true; }        
    bool        canManageUsers()    const override { return true; }        
    bool        canSendNotifs()     const override { return true; }         
    bool        canViewStats()      const override { return true; }       
    std::string getDashboardPath()  const override { return "/admin/dashboard.html"; }
};
