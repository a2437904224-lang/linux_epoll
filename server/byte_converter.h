#ifndef BYTE_CONVERTER_H
#define BYTE_CONVERTER_H

#include <stdint.h>

// 字节序枚举
enum class ByteOrder {
    LittleEndian,  // 小端字节序
    BigEndian      // 大端字节序
};

// 字节序转换类
class ByteConverter {
public:
    ByteConverter();
    ~ByteConverter();
    
    // 设置字节序
    void SetByteOrder(ByteOrder order);
    
    // 获取当前字节序
    ByteOrder GetByteOrder() const;
    
    // 获取主机字节序
    static ByteOrder GetHostByteOrder();
    
    // 16位整数转换
    uint16_t Convert16(uint16_t value) const;
    
    // 32位整数转换
    uint32_t Convert32(uint32_t value) const;
    
    // 64位整数转换
    uint64_t Convert64(uint64_t value) const;
    
private:
    ByteOrder m_byte_order;  // 当前字节序
    ByteOrder m_host_order;  // 主机字节序
    
    // 字节交换函数
    static uint16_t Swap16(uint16_t value);
    static uint32_t Swap32(uint32_t value);
    static uint64_t Swap64(uint64_t value);
};

#endif // BYTE_CONVERTER_H