#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;

class groupModel
{
public:
    // 创建群组
    bool creatGroup(Group &group);

    // 加入群组
    bool joinGroup(int userid, int groupid, string role);

    // 查询用户所在的群组信息
    vector<Group> queryGroups(int userid);

    // 根据指定的groupid查询群组用户id列表，除了userid自己，
    // 主要是为了用户群聊业务，给群组其他成员发送群消息
    vector<int> queryGroupUsers(int userid, int groupid); 

};




#endif