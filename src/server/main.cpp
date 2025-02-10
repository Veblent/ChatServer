#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

// 处理服务器被SIGINT信号结束时，要重置User的state=offline
void resetHandler(int signo)
{
    ChatService::getInstance()->reset();
    exit(0);
}


int main(int argc, char ** argv)
{

    if(argc < 3)
    {
        cerr << "启动服务器命令错误，正确示例如：./ChatServer 127.0.0.1 6000" << endl;
        return -1;
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = resetHandler;
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}