#ifndef CHATSERVER_H
#define CHARSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;


// 聊天服务器的主类
class ChatServer
{
public:
    
    // 初始化聊天服务器对象
    ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg);

    // 启动服务
    void start();

private:
    TcpServer _server;      // 组合的muduo库
    EventLoop *_loop;       // 指向事件循环的指针
    
    // 连接建立和断开事件的回调函数
    void onConnect(const TcpConnectionPtr& conn);

    // 数据读写事件的回调函数
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime);
};


#endif