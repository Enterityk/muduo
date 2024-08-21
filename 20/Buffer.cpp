#include "Buffer.h"

/*
class Buffer
{
private:
    std::string buf_;                // 用于存放数据。

public:
    Buffer();
    ~Buffer();

    void append(const char *data,size_t size);             // 把数据追加到buf_中。
    size_t size();                                                            // 返回buf_的大小。
    const char *data();                                                  // 返回buf_的首地址。
    void clear();                                                            // 清空buf_。
};
*/

Buffer::Buffer()
{

}

Buffer::~Buffer()
{

}

// 把数据追加到buf_中。
void Buffer::append(const char *data,size_t size)             
{
    buf_.append(data,size);
}

// 从buf_的pos开始，删除nn个字节，pos从0开始。
void Buffer::erase(size_t pos,size_t nn)                             
{
    buf_.erase(pos,nn);
}

// 返回buf_的大小。
size_t Buffer::size()                                                            
{
    return buf_.size();
}

// 返回buf_的首地址。
const char *Buffer::data()                                                  
{
    return buf_.data();
}

// 清空buf_。
void Buffer::clear()                                                            
{
    buf_.clear();
}