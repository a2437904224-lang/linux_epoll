#include "byte_converter.h"

ByteConverter::ByteConverter() 
    : m_byte_order(ByteOrder::BigEndian), 
      m_host_order(GetHostByteOrder()) {
}

ByteConverter::~ByteConverter() {
}

void ByteConverter::SetByteOrder(ByteOrder order) {
    m_byte_order = order;
}

ByteOrder ByteConverter::GetByteOrder() const {
    return m_byte_order;
}

ByteOrder ByteConverter::GetHostByteOrder() {
    union {
        uint16_t value;
        uint8_t bytes[2];
    } test = {0x0102};
    
    // 如果低地址存储低位字节，则为小端字节序
    return (test.bytes[0] == 0x02) ? ByteOrder::LittleEndian : ByteOrder::BigEndian;
}

uint16_t ByteConverter::Convert16(uint16_t value) const {
    // 如果主机字节序与目标字节序相同，则不需要转换
    if (m_host_order == m_byte_order) {
        return value;
    }
    
    // 否则交换字节序
    return Swap16(value);
}

uint32_t ByteConverter::Convert32(uint32_t value) const {
    // 如果主机字节序与目标字节序相同，则不需要转换
    if (m_host_order == m_byte_order) {
        return value;
    }
    
    // 否则交换字节序
    return Swap32(value);
}

uint64_t ByteConverter::Convert64(uint64_t value) const {
    // 如果主机字节序与目标字节序相同，则不需要转换
    if (m_host_order == m_byte_order) {
        return value;
    }
    
    // 否则交换字节序
    return Swap64(value);
}

uint16_t ByteConverter::Swap16(uint16_t value) {
    return ((value & 0x00FF) << 8) | 
           ((value & 0xFF00) >> 8);
}

uint32_t ByteConverter::Swap32(uint32_t value) {
    return ((value & 0x000000FF) << 24) | 
           ((value & 0x0000FF00) << 8)  | 
           ((value & 0x00FF0000) >> 8)  | 
           ((value & 0xFF000000) >> 24);
}

uint64_t ByteConverter::Swap64(uint64_t value) {
    return ((value & 0x00000000000000FFULL) << 56) | 
           ((value & 0x000000000000FF00ULL) << 40) | 
           ((value & 0x0000000000FF0000ULL) << 24) | 
           ((value & 0x00000000FF000000ULL) << 8)  | 
           ((value & 0x000000FF00000000ULL) >> 8)  | 
           ((value & 0x0000FF0000000000ULL) >> 24) | 
           ((value & 0x00FF000000000000ULL) >> 40) | 
           ((value & 0xFF00000000000000ULL) >> 56);
}