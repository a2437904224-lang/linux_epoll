#ifndef TLV_PROTOCOL_H
#define TLV_PROTOCOL_H

#include <stdint.h>
#include <vector>
#include <string>
#include "byte_converter.h"

// TLV消息结构
struct TLVMessage {
    uint16_t type;           // 消息类型
    uint32_t length;         // 消息长度
    std::vector<char> value; // 消息内容
    
    TLVMessage() : type(0), length(0) {}
    
    TLVMessage(uint16_t t, const char* v, uint32_t l) 
        : type(t), length(l) {
        value.assign(v, v + l);
    }
};

// TLV协议处理类
class TLVProtocol {
public:
    TLVProtocol();
    ~TLVProtocol();
    
    // 解析TLV消息
    bool ParseMessage(const char* data, size_t len, TLVMessage& msg, size_t& consumed);
    
    // 序列化TLV消息
    bool SerializeMessage(const TLVMessage& msg, std::vector<char>& output);
    
    // 设置字节序（默认为网络字节序，即大端）
    void SetByteOrder(ByteOrder order);
    
private:
    ByteConverter m_converter; // 字节序转换器
    
    // TLV头部大小（类型2字节 + 长度4字节）
    static const size_t TLV_HEADER_SIZE = 6;
};

#endif // TLV_PROTOCOL_H