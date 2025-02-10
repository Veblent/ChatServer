#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/

enum EnMsgType
{
    LOGIN_MSG       = 1,    // 登录消息
    LOGIN_ACK_MSG   = 2,    // 登录响应消息
    REG_MSG         = 3,    // 注册消息
    REG_ACK_MSG     = 4,    // 注册响应消息
    ONE_CHAT_MSG    = 5,    // 聊天消息
    ADD_FRIEND_MSG  = 6,    // 添加好友消息

    CREATE_GROUP_MSG    = 7,    // 创建群
    JOIN_GROUP_MSG      = 8,    // 加入群
    GROUP_CHAT_MSG      = 9,    // 群聊天

    LOGOUT_MSG      =   10, // 退出消息
    LOGOUT_ACK_MSG  =   11, // 退出响应
};

#endif