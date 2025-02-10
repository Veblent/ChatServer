#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include <functional>
using namespace std;
using namespace placeholders;
using json = nlohmann::json; 

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册连接处理回调函数
    _server.setConnectionCallback(bind(&ChatServer::onConnect, this, _1));

    // 注册消息处理回调函数
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}


void ChatServer::start()
{
    _server.start();
}

// 上报连接相关信息的回调函数
void ChatServer::onConnect(const TcpConnectionPtr& conn)
{
    // 客户端断开
    if(!conn->connected())
    {
        ChatService::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime)
{
    string buf = buffer->retrieveAllAsString();
    
    // 数据的反序列化
    json js = json::parse(buf);

    // 通过js["msgid"]获取业务handler，实现网络模块代码与业务模块代码的完全解耦
    MsgHandler handler = ChatService::getInstance()->getHandler(js["msgid"].get<int>());

    // 调用对应msgid的回调函数，来执行相应的业务
    handler(conn, js, receiveTime);
}
