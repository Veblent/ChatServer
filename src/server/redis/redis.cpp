#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis()
{
    _publish_context = nullptr;
    _subscribe_context = nullptr;
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "redis连接失败！<_publish_context>" << endl;
        return false;
    }
    redisReply * reply = (redisReply *)redisCommand(_publish_context, "AUTH %s", "001104zhang");
    if (reply->type == REDIS_REPLY_ERROR)
    {
        cerr << "redis认证失败！<_publish_context>" << endl;
        return false;
    }
    else
    {
        cout << "redis认证成功！<_publish_context>" << endl;
    }

    // 负责subscribe订阅消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (_subscribe_context == nullptr)
    {
        cerr << "redis连接失败！<_subscribe_context>" << endl;
        return false;
    }
    reply = (redisReply *)redisCommand(_subscribe_context, "AUTH %s", "001104zhang");
    if (reply->type == REDIS_REPLY_ERROR)
    {
        cerr << "redis认证失败！<_subscribe_context>" << endl;
        return false;
    }
    else
    {
        cout << "redis认证成功！<_subscribe_context>" << endl;
    }

    freeReplyObject(reply);


    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&]() { observer_channel_msg(); });
    t.detach();

    cout << "redis连接成功！" << endl;

    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string msg)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, msg.c_str());
    if (reply == nullptr)
    {
        cerr << "publish命令执行失败！" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 从redis指定的通道订阅消息
bool Redis::subscribe(int channel)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待消息里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server的响应，否则notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe命令发送失败" << endl;
        return false;
    }
    else
    {
        cout << "成功发送命令：SUBSCRIBE " << channel << endl;
    }

    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(_subscribe_context, &done))
        {
            cerr << "subscribe命令执行失败" << endl;
            return false;
        }
    }

    // redisGetReply阻塞获取响应
    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe命令发送失败！" << endl;
        return false;
    }

    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(_subscribe_context, &done))
        {
            cerr << "unsubscribe命令执行失败！" << endl;
            return false;
        }
    }

    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_msg()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(_subscribe_context, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>_notify_message_handler结束<<<<<<<<<<" << endl;
}

void Redis::init_notify_handler(function<void(int, string)> fn)
{
    _notify_message_handler = fn;
}