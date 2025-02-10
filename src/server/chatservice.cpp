#include "chatservice.hpp"
#include "public.hpp"
#include <string>
#include <vector>
#include <muduo/base/Logging.h>
using namespace muduo;
using namespace std;

// 获取单例对象的接口函数
ChatService *ChatService::getInstance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({JOIN_GROUP_MSG, bind(&ChatService::joinGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, bind(&ChatService::logout, this, _1, _2, _3)});


    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息回调
        _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMsg, this, _1, _2));
        cout << "设置上报消息回调" << endl;
    }

}

ChatService::ChatService(const ChatService &service)
{
}

void ChatService::reset()
{
    // 把online用户状态设置为offline
    if (_userModel.resetState())
    {
        LOG_INFO << "服务器异常终止，已成功将所有用户下线";
    }
    else
    {
        LOG_ERROR << "服务器异常终止，强制用户下线失败！";
    }
}

// 获取消息对应的handler
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志：msgid没有对应的事件处理回调函数
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp receiveTime)
        {
            LOG_ERROR << "msgid = " << msgid << "; 异常msgid，不存在对应业务处理函数！";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务     ORM（对象关系映射）：业务层操作的都是对象
// {"msgid":1, "id":1, "password":"123456"}
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp receiveTime)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);

    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_ACK_MSG;
            response["errno"] = 2;
            response["errmsg"] = "该账户已上线，请勿重复登录！";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            // 用户多线程登录，同时向_userConnMap中插入信息，需要注意线程安全
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // 登录成功，向redis订阅channel(id)
            if(_redis.subscribe(id))
            {
                cout << "订阅成功，channel = " << id << endl;
            }
            else
            {
                cout << "订阅失败，channel = " << id << endl;
            }

            // 登录成功，更新用户状态信息
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_ACK_MSG;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                // 读取该用户的离线消息
                response["offlinemsg"] = vec;
                // 读取完成后，清空该用户的所有离线消息
                _offlineMsgModel.clear(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _frinedModel.query(id);
            if (!userVec.empty())
            {
                vector<string> friendsVec;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    friendsVec.push_back(js.dump());
                }
                response["friends"] = friendsVec;
            }

            // 查询该用户的群列表
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if(!groupVec.empty())
            {
                vector<string> groupStrVec;
                for(Group &group : groupVec)
                {
                    json js;
                    js["id"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();

                    vector<string> gUsers;
                    for(groupUser &gUser : group.getUsers())
                    {
                        json js;
                        js["id"] = gUser.getId();
                        js["name"] = gUser.getName();
                        js["state"] = gUser.getState();
                        js["role"] = gUser.getRole();
                        gUsers.push_back(js.dump());
                    }

                    js["groupusers"] = gUsers;
                    groupStrVec.push_back(js.dump());

                }
                response["groups"] = groupStrVec;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在，或密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_ACK_MSG;
        response["errno"] = 1;
        response["errmsg"] = "账号或密码错误！";
        conn->send(response.dump());
    }
}

// 处理注册业务：name password
// {"msgid":3, "name":"li feng", "password":"123456"}
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp receiveTime)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);

    bool ret = _userModel.insert(user);
    if (ret)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_ACK_MSG;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_ACK_MSG;
        response["errno"] = 1;
        response["errmsg"] = "注册失败，请重试！";
        conn->send(response.dump());
    }
}

// 一对一聊天业务
// {"msgid":5, "fromid":1, "fromname":"zhangsan", "toid":2, "time":"2025.1.15", "msg": "hello, lisi "}
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp receiveTime)
{
    int toId = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toId);
        if (it != _userConnMap.end())
        {
            // 对方用户在线：转发消息；服务器主动推送消息给对方用户（在同一台服务器上登录）
            cout << "对方在同台服务器登录，直接转发" << endl;
            it->second->send(js.dump());
            return;
        }
    }

    // 查询用户是否在线
    User user = _userModel.query(toId);
    if(user.getState() == "online")
    {
        cout << toId << "用户在其他服务器登录，通过redis转发。" << endl;
        if(_redis.publish(toId, js.dump()))
        {
            cout << "publish成功！channel = " << toId << endl;
        }
        else
        {
            cout << "publish失败！channel = " << toId << endl;
        }
        return ;
    }

    // 对方用户离线，存储离线消息
    _offlineMsgModel.insert(toId, js.dump());
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            } 
        }
    }

    _redis.unsubscribe(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}


// 用户正常退出登录
void ChatService::logout(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime)
{
    int userID = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userID);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    
    // 用户注销，相当于下线，在redis中取消订阅通道
    if(_redis.unsubscribe(userID))
    {
        cout << "成功取消订阅：channel = " << userID << endl;
    }

    User user;
    user.setId(userID);
    if(user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);

        json response;
        response["msgid"] = LOGOUT_ACK_MSG;
        response["id"] = userID;
        conn->send(response.dump());
    }
}



// 添加好友业务：msgid, id, friendid
// {"msgid":6, "id":1, "friendid":4}
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp receiveTime)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _frinedModel.insert(userid, friendid);
}

// 创建群
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp receiveTime)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);
    if(_groupModel.creatGroup(group))
    {
        _groupModel.joinGroup(userid, group.getId(), "creator");
        LOG_INFO << "成功创建新群聊！群账号为：" << group.getId() << "群名为：" << name << "，创建者账号为：" << userid;
    }
    else
    {
        LOG_ERROR << "创建新群聊失败！";
    }
}

// 加入群
void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp receiveTime)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.joinGroup(userid, groupid, "normal");
}

// 群聊天
// {"msgid":9, "fromid":1, "fromname":"zhangsan", "groupid":2, "groupname":"group1", "time":"2025.1.15", "msg": "hello, everyone"}
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp receiveTime)
{
    int userid = js["fromid"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> toIdsVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for(int id : toIdsVec)
    {
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                // 用户在其他服务器登录
                _redis.publish(id, js.dump());
            }
            else
            {
                // 用户不在线，存储离线消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}


// 从消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMsg(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }
    // 在该服务器不在线
    _offlineMsgModel.insert(userid, msg);
}