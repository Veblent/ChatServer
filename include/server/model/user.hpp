#ifndef USER_H
#define USER_H
#include <string>
using namespace std;

// User表的ORM类，映射表的各个字段到User对象

/*
ORM框架：
    使用面向对象的方式来操作数据库
    将数据库表映射到编程语言中的类
    将表中的记录映射到类的实例（对象）
    将表的字段映射到对象的属性
*/


class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        this->_id = id;
        this->_name = name;
        this->_password = pwd;
        this->_state = state;
    }

    void setId(int id) { this->_id = id; }
    void setName(string name) { this->_name = name; }
    void setPwd(string pwd) { this->_password = pwd; }
    void setState(string state) { this->_state = state; }

    int getId() { return this->_id; }
    string getName() { return this->_name; }
    string getPwd() { return this->_password; }
    string getState() { return this->_state; }

protected:
    int _id;
    string _name;
    string _password;
    string _state;
};

#endif