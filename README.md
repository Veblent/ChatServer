# ChatServer
可以工作在Nginx TCP负载均衡环境中的集群服务器和客户端源代码（网络模块是基于开源的muduo网络库实现）

### 编译：
```
cd build
rm -rf *
cmake ..
make
```

### 启动多个服务器程序
需要Nginx TCP负载均衡配置
```
./bin/CharServer 127.0.0.1 6000
./bin/CharServer 127.0.0.1 6002
```

###  启动客户端程序
```
./bin/ChatClient 127.0.0.1 8000
```

在客户端按照提示可以执行聊天命令
