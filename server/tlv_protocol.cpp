#include "tlv_protocol.h"
#include <cstring>

TLVProtocol::TLVProtocol() {
    // 默认使用网络字节序（大端）
    m_converter.SetByteOrder(ByteOrder::BigEndian);
}

TLVProtocol::~TLVProtocol() {
}

bool TLVProtocol::ParseMessage(const char* data, size_t len, TLVMessage& msg, size_t& consumed) {
    // 检查数据长度是否足够解析头部
    if (len < TLV_HEADER_SIZE) {
        consumed = 0;
        return false;
    }
    
    // 解析类型（2字节）
    uint16_t type;
    memcpy(&type, data, sizeof(type));
    msg.type = m_converter.Convert16(type);
    
    // 解析长度（4字节）
    uint32_t length;
    memcpy(&length, data + sizeof(type), sizeof(length));
    msg.length = m_converter.Convert32(length);
    
    // 检查数据长度是否足够解析完整消息
    if (len < TLV_HEADER_SIZE + msg.length) {
        consumed = 0;
        return false;
    }
    
    // 解析值
    msg.value.assign(data + TLV_HEADER_SIZE, data + TLV_HEADER_SIZE + msg.length);
    
    // 设置已消费的字节数
    consumed = TLV_HEADER_SIZE + msg.length;
    
    return true;
}

bool TLVProtocol::SerializeMessage(const TLVMessage& msg, std::vector<char>& output) {
    // 计算总长度
    size_t total_size = TLV_HEADER_SIZE + msg.length;
    
    // 调整输出缓冲区大小   
    output.resize(total_size);
    
    // 序列化类型（2字节）
    uint16_t type = m_converter.Convert16(msg.type);
    memcpy(output.data(), &type, sizeof(type));
    
    // 序列化长度（4字节）
    uint32_t length = m_converter.Convert32(msg.length);
    memcpy(output.data() + sizeof(type), &length, sizeof(length));
    
    // 序列化值
    if (msg.length > 0) {
        memcpy(output.data() + TLV_HEADER_SIZE, msg.value.data(), msg.length);
    }
    
    return true;
}

void TLVProtocol::SetByteOrder(ByteOrder order) {
    m_converter.SetByteOrder(order);
}