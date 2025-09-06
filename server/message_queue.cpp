#include "message_queue.h"

MessageQueue::MessageQueue() {
}

MessageQueue::~MessageQueue() {
    ClearAll();
}

bool MessageQueue::Push(int fd, const char* data, size_t len) {
    if (fd < 0 || !data || len == 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queues[fd].push(MessageEntry(data, len));
    
    return true;
}

bool MessageQueue::PushFront(int fd, const char* data, size_t len) {
    if (fd < 0 || !data || len == 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 创建一个新队列，先放入新消息，再放入原队列中的所有消息
    std::queue<MessageEntry> new_queue;
    new_queue.push(MessageEntry(data, len));
    
    auto& old_queue = m_queues[fd];
    while (!old_queue.empty()) {
        new_queue.push(std::move(old_queue.front()));
        old_queue.pop();
    }
    
    // 替换原队列
    m_queues[fd] = std::move(new_queue);
    
    return true;
}

bool MessageQueue::GetMessages(int fd, std::vector<char>& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_queues.find(fd);
    if (it == m_queues.end() || it->second.empty()) {
        return false;
    }
    
    // 计算总数据大小
    size_t total_size = 0;
    std::queue<MessageEntry> temp_queue = it->second;
    while (!temp_queue.empty()) {
        total_size += temp_queue.front().data.size();
        temp_queue.pop();
    }
    
    // 调整输出缓冲区大小
    data.clear();
    data.reserve(total_size);
    
    // 合并所有消息
    auto& queue = it->second;
    while (!queue.empty()) {
        const auto& entry = queue.front();
        data.insert(data.end(), entry.data.begin(), entry.data.end());
        queue.pop();
    }
    
    return true;
}
//用于判断某个连接是否有待发送的数据，常用于发送线程或 epoll 写事件处理时决定是否需要发送消息。
bool MessageQueue::HasMessages(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_queues.find(fd);
    return (it != m_queues.end() && !it->second.empty());
}

/**
 * @brief 获取所有具有非空消息队列的文件描述符（fd）。
 *
 * 该函数会加锁保护消息队列的数据结构，遍历所有队列，
 * 并收集那些消息队列不为空的文件描述符（fd）。
 *
 * @return std::vector<int> 包含所有对应非空消息队列的文件描述符。
 *
 * @note 线程安全：函数内部使用互斥锁保护共享资源。
 */
std::vector<int> MessageQueue::GetAllFds() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<int> fds;
    for (const auto& pair : m_queues) {
        if (!pair.second.empty()) {
            fds.push_back(pair.first);
        }
    }
    
    return fds;
}

void MessageQueue::Clear(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_queues.find(fd);
    if (it != m_queues.end()) {
        // 清空队列
        std::queue<MessageEntry> empty;
        std::swap(it->second, empty);
        
        // 移除空队列
        m_queues.erase(it);
    }
}

void MessageQueue::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queues.clear();
}