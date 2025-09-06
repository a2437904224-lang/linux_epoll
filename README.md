# C++ Epoll Server

这是一个基于Linux epoll的高性能C++网络服务器，实现了TLV（Type-Length-Value）协议进行通信。

## 功能特性

- **高性能I/O**: 使用 `epoll` 实现高并发的网络连接处理。
- **TLV协议**: 内置了对TLV消息格式的解析和序列化，方便进行结构化数据传输。
- **异步消息发送**: 通过线程安全的消息队列实现异步数据发送。
- **字节序处理**: 包含字节序转换工具，确保跨平台通信的正确性。
- **回调机制**: 简洁的API，通过回调函数处理新连接、断开连接和消息接收事件。

## 项目结构

## 如何编译和运行

该项目使用 `Makefile` 进行构建。

1. **编译项目**:

   ```sh
   make
   ```

2. **运行服务器**:

   服务器可以接受一个可选的IP地址和端口号作为命令行参数。如果不提供，将使用默认值 `0.0.0.0:8888`。

   - **使用默认配置启动**:

     ```sh
     ./epoll_server
     ```

   - **指定IP和端口启动**:

     ```sh
     ./epoll_server <ip_address> <port>
     ```

     例如:

     ```sh
     ./epoll_server 127.0.0.1 9999
     ```

3. **清理生成文件**:

   ```sh
   make clean
   ```

## 使用方法

`main.cpp` 中提供了一个基本的使用示例，展示了如何启动服务器并处理消息。

1. **初始化服务器**:

   ```cpp
   EpollServer server("127.0.0.1", 8080);
   ```

2. **设置回调函数**:

   ```cpp
   server.SetOnConnectCallback(OnConnect);
   server.SetOnDisconnectCallback(OnDisconnect);
   server.SetOnMessageCallback(OnMessage);
   ```

3. **启动服务器**:

   ```cpp
   if (server.Start()) {
       // 服务器运行中...
   }
   ```

## 核心组件

- `EpollServer`: 封装了 `epoll` 的核心逻辑，管理客户端连接和事件循环。
- `TLVProtocol`: 用于将原始字节流解析为 `TLVMessage` 结构体，或将 `TLVMessage` 序列化为字节流。
- `MessageQueue`: 一个线程安全的消息队列，用于缓存待发送的数据。
- `ByteConverter`: 处理网络字节序和主机字节序之间的转换。

## 注意

- 本项目依赖于Linux环境下的 `epoll` API，因此无法在Windows上直接编译运行。
