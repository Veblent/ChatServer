#ifndef GROUP_H
#define GROUP_H
#include <iostream>
#include <vector>
#include "groupuser.hpp"
using namespace std;

class Group{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        _id = id;
        _groupName = name;
        _desc = desc;
    }

    void setId(int id) {_id = id;}
    void setName(string name) {_groupName = name;}
    void setDesc(string desc) {_desc = desc;}

    int getId() {return _id;}
    string getName() {return _groupName;}
    string getDesc() {return _desc;}
    vector<groupUser> &getUsers() {return _users;}

private:
    int _id;
    string _groupName;
    string _desc;
    vector<groupUser> _users;
};

#endif