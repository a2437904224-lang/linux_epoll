#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <map>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

// 消息队列类，用于异步发送
class MessageQueue {
public:
    MessageQueue();
    ~MessageQueue();
    
    // 将消息添加到队列尾部
    bool Push(int fd, const char* data, size_t len);
    
    // 将消息添加到队列头部（优先发送）
    bool PushFront(int fd, const char* data, size_t len);
    
    // 获取指定fd的所有消息
    bool GetMessages(int fd, std::vector<char>& data);
    
    // 检查指定fd是否有消息
    bool HasMessages(int fd);
    
    // 获取所有有消息的fd
    std::vector<int> GetAllFds();
    
    // 清空指定fd的消息队列
    void Clear(int fd);
    
    // 清空所有消息队列
    void ClearAll();
    
private:
    // 消息队列结构
    struct MessageEntry {
        std::vector<char> data;
        
        MessageEntry(const char* d, size_t len) {
            data.assign(d, d + len);
        }
    };
    
    // 每个fd对应一个消息队列
    std::map<int, std::queue<MessageEntry>> m_queues;
    
    // 互斥锁，保证线程安全
    std::mutex m_mutex;
};

#endif // MESSAGE_QUEUE_H