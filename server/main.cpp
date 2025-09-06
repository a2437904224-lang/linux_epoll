#include <iostream>
#include <string>
#include <signal.h>
#include "epoll_server.h"

// 全局服务器实例
EpollServer* g_server = nullptr;

// 信号处理函数
void SignalHandler(int sig) {
    if (g_server) {
        std::cout << "Stopping server..." << std::endl;
        g_server->Stop();
    }
    
    exit(0);
}

// 消息处理回调
void OnMessage(int client_fd, const TLVMessage& msg) {
    std::cout << "Received message from client " << client_fd 
              << ", type: " << msg.type 
              << ", length: " << msg.length << std::endl;
    
    // 简单的回显服务，将收到的消息发送回客户端
    if (g_server) {
        // 创建响应消息
        TLVMessage response;
        response.type = msg.type + 1;  // 响应类型为请求类型+1
        response.length = msg.length;
        response.value = msg.value;
        
        // 序列化消息
        TLVProtocol protocol;
        std::vector<char> data;
        if (protocol.SerializeMessage(response, data)) {
            // 发送响应
            g_server->SendMessage(client_fd, data.data(), data.size());
            std::cout << "Sent response to client " << client_fd << std::endl;
        }
    }
}

// 连接回调
void OnConnect(int client_fd) {
    std::cout << "Client connected: " << client_fd << std::endl;
}

// 断开连接回调
void OnDisconnect(int client_fd) {
    std::cout << "Client disconnected: " << client_fd << std::endl;
}

int main(int argc, char* argv[]) {
    // 默认参数
    std::string ip = "0.0.0.0";
    int port = 8888;
    
    // 解析命令行参数
    if (argc > 1) {
        ip = argv[1];
    }
    
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }
    
    // 注册信号处理函数
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    // 创建服务器实例
    g_server = new EpollServer(ip.c_str(), port);
    
    // 设置回调函数
    g_server->SetOnConnectCallback(OnConnect);
    g_server->SetOnDisconnectCallback(OnDisconnect);
    g_server->SetOnMessageCallback(OnMessage);
    
    // 启动服务器
    if (!g_server->Start()) {
        std::cerr << "Failed to start server" << std::endl;
        delete g_server;
        return 1;
    }
    
    std::cout << "Server started on " << ip << ":" << port << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // 主线程等待，实际工作由服务器的工作线程完成
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // 清理资源
    delete g_server;
    
    return 0;
}