#include "epoll_server.h"
#include <iostream>

/**
 * @brief EpollServer类的构造函数
 * @param ip 服务器要绑定的IP地址
 * @param port 服务器要监听的端口号
 * @param max_conn 服务器支持的最大连接数
 * 
 * 初始化成员变量:
 * - m_ip: 存储服务器IP地址
 * - m_port: 存储服务器端口号
 * - m_max_connections: 存储最大连接数限制
 * - m_listen_fd: 监听socket文件描述符，初始化为-1表示未创建
 * - m_epoll_fd: epoll实例文件描述符，初始化为-1表示未创建
 * - m_running: 服务器运行状态标志，初始化为false表示未运行
 */
EpollServer::EpollServer(const char* ip, int port, int max_conn)
    : m_ip(ip), m_port(port), m_max_connections(max_conn),
      m_listen_fd(-1), m_epoll_fd(-1), m_running(false) {


}


EpollServer::~EpollServer() {
    Stop();
}

/**
 * @brief 启动服务器
 * @return 启动成功返回true，失败返回false
 * 
 * 该函数负责启动服务器，主要完成以下工作:
 * 1. 检查服务器是否已经在运行
 * 2. 初始化服务器(创建socket、绑定地址、创建epoll实例等)
 * 3. 启动工作线程
 */
bool EpollServer::Start() {
    // 如果服务器已经在运行，直接返回true
    if (m_running) {
        return true;
    }
    
    // 调用Init()进行服务器初始化，包括:
    // - 创建监听socket
    // - 绑定地址和端口
    // - 创建epoll实例
    // 如果初始化失败，输出错误信息并返回false
    if (!Init()) {
        std::cerr << "Server initialization failed" << std::endl;
        return false;
    }
    
    // 设置服务器运行标志
    m_running = true;
    
    // 创建并启动epoll事件循环线程
    // 该线程负责处理所有的IO事件(新连接、数据读写等)
    m_epoll_thread = std::thread(&EpollServer::EpollLoop, this);
    
    // 创建并启动发送线程
    // 该线程负责处理消息发送队列，确保数据能够及时发送出去
    m_send_thread = std::thread(&EpollServer::SendThread, this);
    
    // 输出服务器启动成功的信息，显示监听的IP和端口
    std::cout << "Server started on " << m_ip << ":" << m_port << std::endl;
    return true;
}

void EpollServer::Stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // 等待线程结束
    if (m_epoll_thread.joinable()) {
        m_epoll_thread.join();
    }
    
    if (m_send_thread.joinable()) {
        m_send_thread.join();
    }
    
    // 关闭epoll
    if (m_epoll_fd != -1) {
        close(m_epoll_fd);
        m_epoll_fd = -1;
    }
    
    // 关闭监听套接字
    if (m_listen_fd != -1) {
        close(m_listen_fd);
        m_listen_fd = -1;
    }
    
    std::cout << "Server stopped" << std::endl;
}

bool EpollServer::Init() {
    // 创建监听套接字
    m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listen_fd == -1) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    // 设置地址重用
    int opt = 1;
    if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
        close(m_listen_fd);
        return false;
    }
    
    // 设置非阻塞
    if (!SetNonBlocking(m_listen_fd)) {
        close(m_listen_fd);
        return false;
    }
    
    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(m_port);
    server_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
    
    if (bind(m_listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind: " << strerror(errno) << std::endl;
        close(m_listen_fd);
        return false;
    }
    
    // 开始监听
    if (listen(m_listen_fd, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        close(m_listen_fd);
        return false;
    }
    
    // 创建epoll实例
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd == -1) {
        std::cerr << "Failed to create epoll: " << strerror(errno) << std::endl;
        close(m_listen_fd);
        return false;
    }
    
    // 添加监听套接字到epoll
    if (!AddToEpoll(m_listen_fd, EPOLLIN)) {
        close(m_epoll_fd);
        close(m_listen_fd);
        return false;
    }
    
    return true;
}

/**
 * @brief 设置文件描述符为非阻塞模式。
 *
 * 此函数通过 fcntl 获取并修改指定文件描述符的标志位，将其设置为非阻塞（O_NONBLOCK）模式。
 * 如果获取或设置标志位失败，会输出错误信息到标准错误流，并返回 false。
 *
 * @param fd 需要设置为非阻塞的文件描述符。
 * @return 如果设置成功返回 true，否则返回 false。
 */
bool EpollServer::SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);//这行代码的作用是获取文件描述符 fd 当前的文件状态标志（flags）
    if (flags == -1) {
        std::cerr << "Failed to get flags: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set non-blocking: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 将指定的文件描述符添加到 epoll 实例进行事件监听。
 *
 * 此函数用于将给定的文件描述符 fd 及其关注的事件类型 events（如 EPOLLIN、EPOLLOUT 等）
 * 添加到 epoll 实例（m_epoll_fd）中。当 epoll_ctl 调用失败时，会在标准错误输出打印错误信息，
 * 并返回 false；成功时返回 true。
 *
 * @param fd      需要添加到 epoll 的文件描述符。
 * @param events  需要监听的事件类型（可以是 EPOLLIN、EPOLLOUT 等的组合）。
 * @return true   添加成功。
 * @return false  添加失败，并输出错误信息。
 */
bool EpollServer::AddToEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        std::cerr << "Failed to add to epoll: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}
//这段代码是用于修改 epoll 监听的事件类型，即动态调整某个文件描述符（fd）在 epoll 中关注的事件
bool EpollServer::ModifyEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        std::cerr << "Failed to modify epoll: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

bool EpollServer::RemoveFromEpoll(int fd) {
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        std::cerr << "Failed to remove from epoll: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

void EpollServer::AcceptConnection() {
    struct sockaddr_in client_addr;//这是什么？ 
    //  struct sockaddr_in 是一个用于存储 IPv4 地址信息的结构体，通常用于网络编程中表示套接字地址。
    socklen_t client_len = sizeof(client_addr);
    
    while (m_running) {
        int client_fd = accept(m_listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 没有新连接了
                break;
            } else {
                std::cerr << "Failed to accept: " << strerror(errno) << std::endl;
                break;
            }
        }
        
        // 设置非阻塞
        if (!SetNonBlocking(client_fd)) {
            close(client_fd);
            continue;
        }
        
        // 添加到epoll
        if (!AddToEpoll(client_fd, EPOLLIN | EPOLLET)) {
            close(client_fd);
            continue;
        }
        
        // 初始化接收缓冲区
        {
            std::lock_guard<std::mutex> lock(m_recv_mutex);
            m_recv_buffers[client_fd] = std::vector<char>();
        }
        
        // 调用连接回调
        if (m_on_connect) {
            m_on_connect(client_fd);/*就会触发 OnConnect 回调，
            也就是执行你在 main.cpp 
            里定义的 OnConnect 函数，实现连接事件通知。*/
        }
        
        std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr) 
                  << ":" << ntohs(client_addr.sin_port) 
                  << " fd: " << client_fd << std::endl;
    }
}

void EpollServer::HandleRead(int fd) {
    char buffer[BUFFER_SIZE];
    
    while (m_running) {
        ssize_t n = read(fd, buffer, BUFFER_SIZE);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 数据读完了
                break;
            } else {
                std::cerr << "Failed to read from fd " << fd << ": " << strerror(errno) << std::endl;
                CloseConnection(fd);
                return;
            }
        } else if (n == 0) {
            // 对端关闭连接
            std::cout << "Connection closed by peer, fd: " << fd << std::endl;
            CloseConnection(fd);
            return;
        }
        
        // 将数据添加到接收缓冲区
        {
            std::lock_guard<std::mutex> lock(m_recv_mutex);
            auto& recv_buffer = m_recv_buffers[fd];
            recv_buffer.insert(recv_buffer.end(), buffer, buffer + n);
            
            // 尝试解析TLV消息
            while (true) {
                TLVMessage msg;
                size_t consumed = 0;
                
                if (m_protocol.ParseMessage(recv_buffer.data(), recv_buffer.size(), msg, consumed)) {
                    // 解析成功，调用消息回调
                    if (m_on_message) {
                        m_on_message(fd, msg);
                    }
                    
                    // 移除已处理的数据
                    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + consumed);
                } else {
                    // 数据不足，等待更多数据
                    break;
                }
            }
        }
    }
}

void EpollServer::HandleWrite(int fd) {
    // 检查是否有数据要发送
    if (m_send_queue.HasMessages(fd)) {
        // 获取要发送的数据
        std::vector<char> data;
        if (m_send_queue.GetMessages(fd, data)) {
            // 发送数据
            ssize_t sent = 0;
            size_t total_sent = 0;
            
            while (total_sent < data.size()) {
                sent = write(fd, data.data() + total_sent, data.size() - total_sent);
                if (sent == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // 发送缓冲区已满，将剩余数据放回队列
                        std::vector<char> remaining(data.begin() + total_sent, data.end());
                        m_send_queue.PushFront(fd, remaining.data(), remaining.size());
                        
                        // 确保监听写事件
                        ModifyEpoll(fd, EPOLLIN | EPOLLOUT | EPOLLET);
                        return;
                    } else {
                        std::cerr << "Failed to write to fd " << fd << ": " << strerror(errno) << std::endl;
                        CloseConnection(fd);
                        return;
                    }
                }
                
                total_sent += sent;
            }
            
            // 如果队列中还有数据，继续监听写事件，否则只监听读事件
            if (m_send_queue.HasMessages(fd)) {
                ModifyEpoll(fd, EPOLLIN | EPOLLOUT | EPOLLET);
            } else {
                ModifyEpoll(fd, EPOLLIN | EPOLLET);
            }
        }
    } else {
        // 没有数据要发送，只监听读事件
        ModifyEpoll(fd, EPOLLIN | EPOLLET);
    }
}

void EpollServer::CloseConnection(int fd) {
    // 从epoll中移除
    RemoveFromEpoll(fd);
    
    // 关闭套接字
    close(fd);
    
    // 清理接收缓冲区
    {
        std::lock_guard<std::mutex> lock(m_recv_mutex);
        m_recv_buffers.erase(fd);
    }
    
    // 清理发送队列
    m_send_queue.Clear(fd);
    
    // 调用断开连接回调
    if (m_on_disconnect) {
        m_on_disconnect(fd);
    }
}

void EpollServer::EpollLoop() {
    struct epoll_event events[MAX_EVENTS];
    
    while (m_running) {
        int nfds = epoll_wait(m_epoll_fd, events, MAX_EVENTS, 100);
        if (nfds == -1) {
            if (errno == EINTR) {
                // 被信号中断，继续
                continue;
            }
            
            std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            
            // 处理错误事件
            //这里 events[i].events 是一个事件掩码，EPOLLERR | EPOLLHUP 是错误和挂起事件的掩码。
            //& 运算会判断 events[i].events 是否包含这两个事件中的任意一个。
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                std::cerr << "epoll error on fd " << fd << std::endl;
                CloseConnection(fd);
                continue;
            }
            
            // 处理监听套接字的读事件（新连接）
            if (fd == m_listen_fd && (events[i].events & EPOLLIN)) {
                AcceptConnection();
                continue;
            }
            
            // 处理客户端套接字的读事件
            if (events[i].events & EPOLLIN) {
                HandleRead(fd);
            }
            
            // 处理客户端套接字的写事件
            if (events[i].events & EPOLLOUT) {
                HandleWrite(fd);
            }
        }
    }
}

void EpollServer::SendThread() {
    while (m_running) {
        // 检查所有连接的发送队列
        std::vector<int> fds = m_send_queue.GetAllFds();
        
        for (int fd : fds) {
            if (m_send_queue.HasMessages(fd)) {
                // 确保监听写事件
                ModifyEpoll(fd, EPOLLIN | EPOLLOUT | EPOLLET);
            }
        }
        
        // 休眠一小段时间，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

/**
 * @brief 向指定客户端发送消息。
 *
 * 此方法用于将待发送的数据添加到发送队列，稍后由服务器线程实际发送给客户端。
 * 发送操作是异步的，调用此方法并不保证数据立即发送完成。
 *
 * @param client_fd 客户端的文件描述符，必须为有效的非负整数。
 * @param data 指向待发送数据的指针。
 * @param len 待发送数据的长度（字节数）。
 * @return 如果服务器正在运行且参数有效，且数据成功加入发送队列，则返回 true；
 *         否则返回 false。
 *
 * @note
 * - 当服务器未运行或 client_fd 无效时，方法直接返回 false。
 * - 数据实际发送由服务器内部机制完成，可能存在延迟。
 * - 发送队列满或发生异常时，Push 可能失败，导致返回 false。
 */
bool EpollServer::SendMessage(int client_fd, const char* data, size_t len) {
    if (!m_running || client_fd < 0) {
        return false;
    }
    
    // 将数据添加到发送队列
    return m_send_queue.Push(client_fd, data, len);
}

void EpollServer::SetOnConnectCallback(std::function<void(int)> callback) {
    m_on_connect = callback;
}

void EpollServer::SetOnDisconnectCallback(std::function<void(int)> callback) {
    m_on_disconnect = callback;
}

void EpollServer::SetOnMessageCallback(std::function<void(int, const TLVMessage&)> callback) {
    m_on_message = callback;
}