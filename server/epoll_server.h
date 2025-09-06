#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

// 系统头文件
#include <sys/epoll.h>      // epoll相关函数
#include <sys/socket.h>     // socket相关函数
#include <netinet/in.h>     // 网络地址结构体
#include <arpa/inet.h>      // IP地址转换函数
#include <fcntl.h>          // 文件控制选项
#include <unistd.h>         // UNIX标准函数
#include <errno.h>          // 错误码
#include <string.h>         // 字符串处理函数
#include <stdlib.h>         // 标准库函数
#include <stdio.h>          // 标准输入输出

// C++标准库
#include <vector>           // 动态数组容器
#include <map>             // 映射容器
#include <thread>          // 线程支持
#include <mutex>           // 互斥量
#include <atomic>          // 原子操作
#include <functional>      // 函数对象

// 自定义头文件
#include "message_queue.h"  // 消息队列
#include "tlv_protocol.h"   // TLV协议

#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096

class EpollServer {
public:
    EpollServer(const char* ip, int port, int max_conn = 1024);
    ~EpollServer();

    // 启动服务器
    bool Start();
    // 停止服务器
    void Stop();
    // 异步发送数据
    bool SendMessage(int client_fd, const char* data, size_t len);
    // 设置连接回调
    void SetOnConnectCallback(std::function<void(int)> callback);
    // 设置断开连接回调
    void SetOnDisconnectCallback(std::function<void(int)> callback);
    // 设置消息回调
    void SetOnMessageCallback(std::function<void(int, const TLVMessage&)> callback);

private:
    // 初始化服务器
    bool Init();
    // 设置非阻塞
    bool SetNonBlocking(int fd);
    // 添加到epoll
    bool AddToEpoll(int fd, uint32_t events);
    // 修改epoll事件
    bool ModifyEpoll(int fd, uint32_t events);
    // 从epoll移除
    bool RemoveFromEpoll(int fd);
    // 接受新连接
    void AcceptConnection();
    // 处理读事件
    void HandleRead(int fd);
    // 处理写事件
    void HandleWrite(int fd);
    // 关闭连接
    void CloseConnection(int fd);
    // Epoll循环
    void EpollLoop();
    // 发送线程函数
    void SendThread();

private:
    std::string m_ip;                // 服务器IP
    int m_port;                      // 服务器端口
    int m_listen_fd;                 // 监听套接字
    int m_epoll_fd;                  // epoll文件描述符
    int m_max_connections;           // 最大连接数
    std::atomic<bool> m_running;     // 运行标志
    
    std::thread m_epoll_thread;      // epoll线程
    std::thread m_send_thread;       // 发送线程
    
    std::map<int, std::vector<char>> m_recv_buffers;  // 接收缓冲区
    std::mutex m_recv_mutex;         // 接收缓冲区互斥锁
    
    MessageQueue m_send_queue;       // 发送队列
    
    TLVProtocol m_protocol;          // TLV协议处理器
    
    // 回调函数
    std::function<void(int)> m_on_connect;
    std::function<void(int)> m_on_disconnect;
    std::function<void(int, const TLVMessage&)> m_on_message;
};

#endif // EPOLL_SERVER_H