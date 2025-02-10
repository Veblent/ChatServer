#ifndef CHATSERVICE_H
#define CHATSERVICE_H


#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "user.hpp"
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json; 

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime)>;

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* getInstance();

    // 服务器异常，业务重置方法
    void reset();

    // 处理登录业务
    void login(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime);

    // 处理注册业务
    void reg(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime);

    // 创建群
    void createGroup(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime);

    // 加入群
    void joinGroup(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime);

    // 群聊天
    void groupChat(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime);

    // 获取消息对应的handler
    MsgHandler getHandler(int msgid);

    // 客户端异常退出
    void clientCloseException(const TcpConnectionPtr&conn);

    // 用户正常退出登录
    void logout(const TcpConnectionPtr&conn, json &js, Timestamp receiveTime);


    void handleRedisSubscribeMsg(int userid, string msg);

private:
    // 私有化构造函数
    ChatService();
    ChatService(const ChatService& service);

    // 存储消息id和其对应的业务处理方法，C++中的map不是线程安全的，要加锁
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    // User数据操作类对象
    userModel _userModel;

    // OfflineMessage数据操作对象
    offlineMsgModel _offlineMsgModel;

    // Friend数据操作对象
    friendModel _frinedModel;

    // Group相关操作对象
    groupModel _groupModel;

    // redis操作对象
    Redis _redis;
};

# endif