#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <map>
#include <csignal>
#include "stat_data.h"

using namespace std;

/*!
   epoll实现的HTTP统计服务器 忽略代码可读性 实现功能为主 SIGINT或关闭时未释放资源 
   
   响应结果可以考虑抽离

   @version v2.0 
*/
int serverSocketFd;
int epollFd;

void signalHandler(int sigalNumber)
{
    cout << "服务端已关闭" << endl;
    // 释放资源 忽略错误 忽略没打开资源
    close(serverSocketFd);
    close(epollFd);
    exit(0);  
}

int main(int argc, char ** argv)
{
    signal(SIGINT, signalHandler);   // ctrl + c
    signal(SIGKILL, signalHandler);  // kill -9 $pid 貌似未生效

    // 0. 初始化数据库 内存 磁盘 目录当前工作目录 忽略错误
    StatData statData("stat.txt");
    // 输入文件流对象读取文件 文件不存在不会报错
    // string filename = "stat.txt";
    // ifstream ifs(filename);
    // string line;
    // // 统计数据
    // map<string, int> stat_data;
    // while (getline(ifs, line)){
    //     // cout << line << endl;
    //     int lastColonIndex = line.rfind("=");
    //     // cout << line.substr(0, lastColonIndex) << "=" << atoi(line.substr(lastColonIndex + 1).c_str())  << endl;
    //     stat_data[line.substr(0, lastColonIndex)] = atoi(line.substr(lastColonIndex + 1).c_str());
    // }
    // ifs.close();

    // 1. 创建socket，监听33333 忽略错误
    int serverPort = 33333;
    struct sockaddr_in serverSocketAddress;
    struct sockaddr_in clientSocketAddress;
    socklen_t clientSocketLen;
    serverSocketAddress.sin_family = AF_INET;
    serverSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY); 
    serverSocketAddress.sin_port = htons(serverPort);
    serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);

    // 解决TIME_WAIT状态引起的bind失败
    // # netstat -anp | grep 33333
    // tcp        0      0 192.168.1.206:33333     192.168.1.205:46154     TIME_WAIT   -  
    int opt = 1;    
    setsockopt(serverSocketFd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

    bind(serverSocketFd, (struct sockaddr *)&serverSocketAddress, sizeof(serverSocketAddress));
    listen(serverSocketFd, 1024);
  
    // 2. 创建epoll，创建epoll_event事件数组 忽略错误
    epollFd = epoll_create(1);
    int maxevents = 1024;
    struct epoll_event * epoll_events = new epoll_event[maxevents];
    // 3. 把服务端socket加入事件数组 忽略错误
    struct epoll_event epoll_event;
    epoll_event.events = EPOLLIN;  // EPOLLIN EPOLL输入 读
    epoll_event.data.fd = serverSocketFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocketFd, &epoll_event);
    cout << "服务端已启动" << endl;
    
    // 4. 事件循环 忽略错误
    for (;;)
    {
        // 阻塞
        int count = epoll_wait(epollFd, epoll_events, maxevents, -1); 
        cout << "------------------------------" << endl;
        cout << "待处理的EPOLL事件数量：" << count << endl;
        for (int i = 0; i < count; ++i)
        {
            int events = epoll_events[i].events;
            int socketFd = epoll_events[i].data.fd;
            // 有可读事件
            if (events & EPOLLIN) 
            {
                // 客户端连接
                if (socketFd == serverSocketFd)
                {
                    // 5. 接受客户端连接 并加入到epoll 忽略错误
                    int newSocketFd = accept(serverSocketFd, (struct sockaddr *)&clientSocketAddress, &clientSocketLen);
                    struct epoll_event epoll_event;
                    epoll_event.events = EPOLLIN;  // EPOLLIN EPOLL输入 读
                    epoll_event.data.fd = newSocketFd;
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, newSocketFd, &epoll_event);

                    string newClientIp = inet_ntoa(clientSocketAddress.sin_addr);
                    // fd 和 ptr在一个 union 里面，只是放一个 ptr可以放更多的数据
                    // epoll_event.data.ptr = (void *)ip.c_str();  
                    cout << "新客户端IP：" << newClientIp << endl;
                }
                // 6. 读取客户端数据 HTTP数据 只取需要部分 忽略错误
                else
                {
                    char readBuffer[1024];
                    int readNum = read(socketFd, readBuffer, sizeof(readBuffer));
                    string readData(readBuffer, readNum);
                    cout << "读取到的数据字节数：" << readNum << endl;
                    // 客户端主动关闭
                    if (readNum == 0) 
                    {
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, socketFd, &epoll_events[i]);
                        close(socketFd);   
                        cout << "客户端主动关闭" << endl;
                    }
                    else 
                    {
                        cout << "读取到的数据：" << endl << readData << endl << endl;
                        /*
                        curl -X POST -d "/project/test1" 192.168.1.206:33333

                        POST / HTTP/1.1
                        User-Agent: curl/7.29.0
                        Host: 192.168.1.206:33333
                        Content-Length: 14
                        Content-Type: application/x-www-form-urlencoded

                        /project/test1
                        */
                        // 7. 解析到项目路径 并统计 忽略异常 忽略没数据等
                        // 如果是GET，响应统计结果，如果是POST，进行统计 不关心 URL /...
                        string responce = "";
                        if (readData.find("GET") != -1) 
                        {
                            string content = statData.get("<br>\n");  // \n是为了调试
                            responce = "HTTP/1.1 200 OK\r\n" 
                                       "Content-Type: text/html\r\n" 
                                       "Content-Length: ";
                            responce += content.length();
                            responce += "\r\n\r\n";
                            responce += content;
                        }
                        else if (readData.find("POST") != -1)
                        {
                            string findStr = "\r\n\r\n";
                            int index = readData.find(findStr);
                            if (index != -1) {
                                string projectPath = readData.substr(readData.find(findStr) + findStr.length());
                                getpeername(socketFd, (struct sockaddr *)&clientSocketAddress, &clientSocketLen);
                                string ip = inet_ntoa(clientSocketAddress.sin_addr);
                                // cout << "项目路径：" << projectPath << endl;
                                // cout << "IP地址：" << ip << endl;
                                string key = ip + "_" + projectPath;
                                statData.incr(key);
                                statData.save();
                                // if (stat_data.find(key) == stat_data.end()) {
                                //     stat_data[key] = 1;
                                // }
                                // else {
                                //     stat_data[key]++;
                                // }
                                // cout << "[统计结果] " << key << " " << stat_data[key] << endl;
                                // 输出文件流对象保存数据
                                // ofstream ofs(filename);
                                // for(auto item : stat_data) {
                                //     line = item.first + "=" + to_string(item.second) + "\n";
                                //     ofs.write(line.c_str(), line.length());
                                // }
                                // ofs.close();
                            }
                            // HTTP响应数据 忽略性能 忽略变量抽离
                            responce = "HTTP/1.1 200 OK\r\n" 
                                    "Content-Type: text/html\r\n" 
                                    "Content-Length: 7\r\n\r\n" 
                                    "success";               
                        }
                 
                        write(socketFd, responce.c_str(), responce.length());
                        // 删除 关闭客户端，以后不需要使用
                        // epoll_ctl(epollFd, EPOLL_CTL_DEL, socketFd, &epoll_events[i]);
                        // close(socketFd);  
                        cout << "响应数据：\n" << responce << endl;
                    }
                }
            }
            // 有可写事件 暂未用到
            else if (events & EPOLLOUT)
            {   
                cout << "有可写事件" << endl;
            }
            // 其他事件 暂未用到
            else 
            {
                cout << "其他事件" << endl;
            }
        }
    }
}