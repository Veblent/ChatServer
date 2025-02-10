#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool groupModel::creatGroup(Group &group)
{
    char sql[1024];
    sprintf(sql, "INSERT INTO AllGroup (groupname, groupdesc) VALUES ('%s', '%s')", group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConn()));
            return true;
        }
    }
    return false;
}

// 加入群组
bool groupModel::joinGroup(int userid, int groupid, string role)
{
    char sql[1024];
    sprintf(sql, "INSERT INTO GroupUser VALUES (%d, %d, '%s')", groupid, userid, role.c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 查询用户所在的群组信息
vector<Group> groupModel::queryGroups(int userid)
{
    char sql[1024];
    sprintf(sql, "SELECT id, groupname, groupdesc \
                  FROM AllGroup JOIN GroupUser ON AllGroup.id=GroupUser.groupid WHERE userid=%d", userid);

    vector<Group> groupVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr)
        {
            Group group;
            group.setId(atoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);

            groupVec.push_back(group);
        }
        mysql_free_result(res);
    }


    // 获取每个组的所有成员信息：成员id 成员name 成员state 成员role
    // 将所有的成员信息依次放入group的vector<groupUser> _user中（getUser返回的是引用）
    for(Group &group : groupVec)
    {
        sprintf(sql, "SELECT User.id, User.name, User.state, GroupUser.grouprole \
                      FROM GroupUser JOIN User ON GroupUser.userid=User.id \
                      WHERE GroupUser.groupid = %d", group.getId());
        
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                groupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);

                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }

    return groupVec;
}

// 根据指定的groupid查询群组用户id列表，除了userid自己，
// 主要是为了用户群聊业务，给群组其他成员发送群消息
vector<int> groupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024];
    sprintf(sql, "SELECT userid FROM GroupUser WHERE groupid = %d AND userid != %d", groupid, userid);

    vector<int> toIdsVec;
    MySQL mysql;
    if(mysql.connect())
    {   
        MYSQL_RES *res =  mysql.query(sql);
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr)
        {
            toIdsVec.push_back(atoi(row[0]));
        }
        mysql_free_result(res);
    }
    return toIdsVec;
}
