#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <iostream>
#include <vector>
using namespace std;

// 提供离线消息表的操作接口方法
class offlineMsgModel
{
public:
    // 存储用户的离线消息
    void insert(int userid, string msg);

    // 清除该用户的离线消息
    void clear(int userid);

    // 查询用户的离线消息
    vector<string> query(int userid);

private:

};

#endif