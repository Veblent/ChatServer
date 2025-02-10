#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

class groupUser : public User
{
public:
    void setRole(string role) {_role = role;}
    string getRole() {return _role;}
private:
    string _role;
};


#endif