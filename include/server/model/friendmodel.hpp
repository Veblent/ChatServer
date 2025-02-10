#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include <vector>
#include "user.hpp"
using namespace std;

// 提供
class friendModel
{

public:

    // 添加好友关系
    void insert(int userid, int friendid);

    // 返回用户好友列表：friendid, name, state
    vector<User> query(int userid);

private:


};


#endif