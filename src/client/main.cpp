#include "json.hpp"
#include <iostream>
#include <pthread.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <thread>
#include <mutex>
using namespace std;
using json = nlohmann::json;


#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

/*
sem_t rwsem;
atomic_bool g_isLogin{false};

sem_init(&rwsem, 0, 0);
sem_destroy(&rwsem);
sem_post(&rwsem);

*/



// 记录当前系统给登录的用户信息
User g_curUser;

// 记录当前登录用户的好友列表
vector<User> g_curUserFriendList;

// 记录当前登录用户的群组列表信息
vector<Group> g_curUserGroupList;

// 显示当前登录成功用户的基本信息
void showCurUseInfo();

// 接受线程
void readTaskHandler(int cfd);

// 获取系统时间（聊天信息需要添加时间信息）
string getCurTime();

// 主聊天页面程序
void mainMenu(int cfd);

void help(int cfd=-1, string str="");

void chat(int, string);

void addFriend(int, string);

void createGroup(int, string);

void joinGroup(int, string);

void groupChat(int, string);

void logout(int, string);


// 聊天客户端程序实现，main线程用作发送线程，子线程用作接受线程
int main(int argc, char **argv)
{
    if(argc < 3)
    {
        cerr << "命令无效！示例：./ChatClient 127.0.0.1 8888" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的IP和PORT
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    sockaddr_in serverAddr;
    memset(&serverAddr, 0x00, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);

    // 创建客户端的socket
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if(cfd == -1)
    {
        cerr << "创建套接字失败！" << endl;
        exit(-1);
    }

    // 客户端与服务端建立连接
    if(-1 == connect(cfd, (sockaddr *)&serverAddr, sizeof(serverAddr)))
    {
        cerr << "建立连接失败！" << endl;
        exit(-1);
    }

    // main线程用于接受用户输入，负责发送数据
    for(;;)
    {
        // 显示首页菜单：登录、注册、退出
        cout << "=====================================" << endl;
        cout << "1. 登录" << endl;
        cout << "2. 注册" << endl;
        cout << "3. 退出" << endl;
        cout << "=====================================" << endl;
        cout << "输入选项：" << endl;
        int option = 0;
        cin >> option;
        cin.get(); // 读出缓冲区残留的回车

        switch(option)
        {
        case 1:
        {
            int id = -1;
            char pwd[64] = {0};
            cout << "请输入账号：";
            cin >> id;
            cin.get(); // 读出缓冲区残留的回车
            cout << "请输入密码：";
            cin.getline(pwd, 64);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int ret = send(cfd, request.c_str(), strlen(request.c_str())+1, 0);
            if(ret == -1)
            {
                cerr << "登录消息发送失败！消息内容：" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                ret = recv(cfd, buffer, sizeof(buffer), 0);
                if(ret == -1)
                {
                    cerr << "登录响应接收失败！" << endl;
                }
                else
                {
                    json response = json::parse(buffer);
                    if(response["errno"] != 0)
                    {
                        // 登录失败
                        cerr << response["errmsg"] << endl;
                    } 
                    else
                    {
                        // 登录成功

                        // 1. 设置当前登录用户的账号和姓名
                        g_curUser.setId(response["id"].get<int>());
                        g_curUser.setName(response["name"]);

                        // 2. 记录当前登录用户的好友列表
                        if(response.contains("friends"))
                        {
                            g_curUserFriendList.clear();
                            vector<string> vec = response["friends"];
                            for(string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);

                                g_curUserFriendList.push_back(user);
                            }
                        }

                        // 3. 记录当前登录用户加入的群列表
                        if(response.contains("groups"))
                        {
                            g_curUserGroupList.clear();
                            vector<string> groupVec = response["groups"];
                            for(string & groupStr : groupVec)
                            {
                                json groupjs = json::parse(groupStr);
                                Group group;
                                group.setId(groupjs["id"].get<int>());
                                group.setName(groupjs["groupname"]);
                                group.setDesc(groupjs["groupdesc"]);

                                vector<string> groupUsersStr = groupjs["groupusers"];
                                for(string & guserStr : groupUsersStr)
                                {
                                    groupUser guser;
                                    json guserjs = json::parse(guserStr);
                                    guser.setId(guserjs["id"].get<int>());
                                    guser.setName(guserjs["name"]);
                                    guser.setState(guserjs["state"]);
                                    guser.setRole(guserjs["role"]);

                                    group.getUsers().push_back(guser);
                                }
                                g_curUserGroupList.push_back(group);
                            }
                        }

                        // 4. 显示登录用户的基本信息
                        showCurUseInfo();

                        // 5. 显示当前用户的离线消息（一对一聊天、群组消息）
                        if(response.contains("offlinemsg"))
                        {
                            vector<string> offlineMsgVec = response["offlinemsg"];
                            cout << "您收到的离线消息如下：" << endl;
                            cout << "-----------------------" << endl;
                            for(string & msg : offlineMsgVec)
                            {
                                json js = json::parse(msg);
                                if(js["msgid"].get<int>() == ONE_CHAT_MSG)
                                {
                                    cout << "好友的消息：\t昵称：" << js["fromname"] << "\t时间：" << js["time"] << "\t" << js["msg"] << endl;
                                }
                                else if (js["msgid"].get<int>() == GROUP_CHAT_MSG)
                                {
                                    cout << "群消息：\t群名:" << js["groupname"] << "\t时间：" << js["time"] 
                                        << "\t发送者：" << js["fromname"] << "\t" << js["msg"] << endl;
                                }
                                cout << "-----------------------" << endl;
                            }
                        }

                        // 6. 登录成功，启动接收线程负责接收数据
                        thread readTask(readTaskHandler, cfd);  // pthread_create
                        readTask.detach();                      // pthread_detach

                        g_curUser.setState("online");

                        // 7. 进入聊天主菜单页面
                        mainMenu(cfd);
                    }
                }
            }
            break;
        }
        case 2:
        {
            char name[64] = {0};
            char pwd[64] = {0};
            cout << "输入用户名：" ;
            cin.getline(name, 64);
            cout << "输入密码：" ;
            cin.getline(pwd, 64);

            json js;
            js["msgid"] = REG_MSG;
            js["name"]  = name;
            js["password"] = pwd;

            string request = js.dump();

            int ret = send(cfd, request.c_str(), strlen(request.c_str())+1, 0);
            if(ret == -1)
            {
                cerr << "注册信息发送失败！消息内容：" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                ret = recv(cfd, buffer, 1024, 0);
                if(ret == -1)
                {
                    cerr << "注册响应接收失败！" << endl;
                }
                else
                {
                    json response = json::parse(buffer);
                    if( 0 != response["errno"].get<int>())
                    {
                        // 注册失败
                        cerr << name << "：" << response["errmsg"] << endl;
                    }
                    else
                    {
                        // 注册成功
                        cout << name << "：注册成功！账号为：" << response["id"] << endl;
                    }
                }
            }
            break;
        }
        case 3:
        {
            close(cfd);
            exit(0);
        }
        default:
        {
            cerr << "无效选项" << endl;
            break; 
        }
        }
    }
}


// 接受线程
void readTaskHandler(int cfd)
{
    while(g_curUser.getState() == "online")
    {
        char buffer[1024] = {0};
        int ret = recv(cfd, buffer, 1024, 0);
        if(ret == -1 || ret == 0)
        {
            close(cfd);
            exit(-1);
        }
        else
        {
            // 接收ChatServer转发的数据，反序列化成json对象
            json js = json::parse(buffer);
            if(js["msgid"] == ONE_CHAT_MSG)
            {
                cout << js["time"] << "来自账号：" << js["fromid"] << " 昵称为：" << js["fromname"] << " 消息内容为：" << js["msg"] << endl;
            }
            else if(js["msgid"] == GROUP_CHAT_MSG)
            {   
                cout << js["time"] << "来自群聊：" << js["groupid"] << " 群名为：" << js["groupname"] 
                    << " 发送人：" << js["fromname"] << " 消息内容：" << js["msg"] << endl;
            }
            else if(js["msgid"] == LOGOUT_ACK_MSG)
            {
                int userID = js["id"].get<int>();
                if(userID == g_curUser.getId())
                {
                    g_curUser.setState("offline");
                    cout << "当前用户：" << g_curUser.getName() << "，已成功退出登录！" << endl;
                }
            }
        }
    }
}

// 系统支持的客户端命令列表
unordered_map<string, string> cmdMap = 
{
    {"help", "\t\t显示功能\t格式：help"},
    {"chat", "\t\t一对一聊天\t格式：chat:friendID:message"},
    {"addFriend", "\t添加好友\t格式：addFriend:friendID"},
    {"createGroup", "\t创建群组\t格式：createGroup:groupName:groupDesc"},
    {"joinGroup", "\t加入群组\t格式：joinGroup:groupID"},
    {"groupChat", "\t群聊\t\t格式：groupChat:groupID:message"},
    {"logout", "\t\t退出\t\t格式：logout"}
};

// 绑定系统支持的客户端命令处理函数
unordered_map<string, function<void(int, string)>> cmdHandlerMap = 
{
    {"help", help},
    {"chat", chat},
    {"addFriend", addFriend},
    {"createGroup", createGroup},
    {"joinGroup", joinGroup},
    {"groupChat", groupChat},
    {"logout", logout}
};

// 主聊天页面程序
void mainMenu(int cfd)
{
    help();
    char buffer[1024] = {0};
    while(g_curUser.getState() == "online")
    {
        cin.getline(buffer, 1024);
        string cmdBuf(buffer);
        string cmd;
        int idx = cmdBuf.find(":");
        if(idx == -1)
        {
            cmd = cmdBuf;
        }
        else
        {
            cmd = cmdBuf.substr(0, idx);
        }
        auto it = cmdHandlerMap.find(cmd);
        if(it == cmdHandlerMap.end())
        {
            cerr << "无效命令！" << endl;
        }
        else
        {
            // 调用相应命令的处理函数，
            it->second(cfd, cmdBuf.substr(idx + 1, cmdBuf.size() - idx));
        }
    }
}


void showCurUseInfo()
{
    cout << "==================login user===================" << endl;
    cout << "当前登录用户账号: " << g_curUser.getId() << " 昵称: " << g_curUser.getName() << endl;
    cout << "------------------friend list-------------------" << endl;
    if(!g_curUserFriendList.empty())
    {
        for(User &user : g_curUserFriendList)
        {
            cout << user.getId() << "\t" << user.getName() << "\t" << user.getState() << endl;
        }
    }
    cout << "------------------group list-------------------" << endl;
    if(!g_curUserGroupList.empty())
    {
        for(Group &group : g_curUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for(groupUser &user : group.getUsers())
            {
                cout << "\t" << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "=====================================" << endl;
}


string getCurTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}


void help(int cfd, string str)
{
    cout << "命令列表如下：" << endl;
    for(auto &it : cmdMap)
    {
        cout << it.first << "\t" << it.second << endl;
    }
    cout << endl;
}


void chat(int cfd, string str)
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr<< "无法解析好友账号，请正确输入命令" << endl;
    }
    else
    {
        int toID = atoi(str.substr(0, idx).c_str());
        bool isfriend = false;
        for(User &friendUser : g_curUserFriendList)
        {
            if(toID == friendUser.getId())
            {
                isfriend = true;
                break;
            }
        }
        
        if(isfriend)
        {
            string msg = str.substr(idx + 1, str.size() - idx);

            json js;
            js["msgid"] = ONE_CHAT_MSG;
            js["fromid"] = g_curUser.getId();
            js["fromname"] = g_curUser.getName();
            js["toid"] = toID;
            js["time"] = getCurTime();
            js["msg"] = msg;

            string buf = js.dump();

            int ret = send(cfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
            if(ret == -1)
            {
                cerr << "聊天消息发送失败：" << buf << endl;
            }
        }
        else
        {
            cerr << "对方不是您的好友，无法发送消息！" << endl;
        }
    }
}

void addFriend(int cfd, string str)
{
    int friendID = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_curUser.getId();
    js["friendid"] = friendID;
    string buf = js.dump();

    int ret = send(cfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if(ret == -1)
    {
        cerr << "添加好友信息发送失败：" << buf << endl;
    }
}

void createGroup(int cfd, string str)
{
    
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr << "无法解析创建群聊的消息，请输入正确命令！" << endl;
    }
    else
    {
        int creatorID = g_curUser.getId();
        string groupname = str.substr(0, idx);
        string groupdesc = str.substr(idx + 1, str.size() - idx);

        json js;
        js["msgid"] = CREATE_GROUP_MSG;
        js["id"] = creatorID;
        js["groupname"] = groupname;
        js["groupdesc"] = groupdesc;

        string buf = js.dump();
        int ret = send(cfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
        if(ret == -1)
        {
            cerr << "创建群组信息发送失败：" << buf << endl;
        }
    }
}

void joinGroup(int cfd, string str)
{
    int groupid = atoi(str.c_str());
    bool joined = false;
    for(Group & group : g_curUserGroupList)
    {
        if(group.getId() == groupid)
        {
            joined = true;
            break;
        }
    }

    if(joined)
    {
        cerr << "你已在该群内，无需重复加入！" << endl;
    }
    else
    {
        json js;
        js["msgid"] = JOIN_GROUP_MSG;
        js["id"] = g_curUser.getId();
        js["groupid"] = groupid;
        
        string buf = js.dump();
        int ret = send(cfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
        if(ret == -1)
        {
            cerr << "加入群组信息发送失败：" << buf << endl;
        }
    }

}

void groupChat(int cfd, string str)
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr << "无法解析群聊的消息，请输入正确命令！" << endl;
    }
    else
    {   
        int fromID = g_curUser.getId();
        string fromName = g_curUser.getName();
        int groupID = atoi(str.substr(0, idx).c_str());
        string msg = str.substr(idx + 1, str.size() - idx).c_str();
        string groupName;
        for(Group &group : g_curUserGroupList)
        {
            if(group.getId() == groupID)
            {
                groupName = group.getName();
            }
        }
        
        json js;
        js["msgid"] = GROUP_CHAT_MSG;
        js["fromid"] = fromID;
        js["fromname"] = fromName;
        js["groupid"] = groupID;
        js["groupname"] = groupName;
        js["time"] = getCurTime();
        js["msg"] = msg;

        string buf = js.dump();
        int ret = send(cfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
        if(ret == -1)
        {
            cerr << "群聊信息发送失败：" << buf << endl;
        }
    }
}

void logout(int cfd, string str)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_curUser.getId();

    string buf = js.dump();
    int ret = send(cfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if(ret == -1)
    {
        cerr << "退出登录消息发送失败！" << endl;
    }
    else
    {
        sleep(1);
    }
}