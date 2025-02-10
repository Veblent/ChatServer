#include "offlinemessagemodel.hpp"
#include "db.h"


// OfflineMessage
void offlineMsgModel::insert(int userid, string msg)
{
    char sql[1204] = {0};
    sprintf(sql, "INSERT INTO OfflineMessage VALUES (%d, '%s')", userid, msg.c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 清除该用户的离线消息
void offlineMsgModel::clear(int userid)
{
    char sql[1204] = {0};
    sprintf(sql, "DELETE FROM OfflineMessage WHERE userid = %d", userid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户的离线消息
vector<string> offlineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql,  "SELECT message FROM OfflineMessage WHERE userid = %d", userid);

    vector<string> vec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {  
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }
        }
        mysql_free_result(res);
    }
    return vec;
}