#include "Connection.h"


Connection::Connection(EventLoop *loop,Socket *clientsock):loop_(loop),clientsock_(clientsock)
{
    // 为新客户端连接准备读事件，并添加到epoll中。
    clientchannel_=new Channel(loop_,clientsock_->fd());   
    clientchannel_->setreadcallback(std::bind(&Connection::onmessage,this));
    clientchannel_->setclosecallback(std::bind(&Connection::closecallback,this));
    clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback,this));
    clientchannel_->setwritecallback(std::bind(&Connection::writecallback,this));
    clientchannel_->useet();                 // 客户端连上来的fd采用边缘触发。
    clientchannel_->enablereading();   // 让epoll_wait()监视clientchannel的读事件
}

Connection::~Connection()
{
    delete clientsock_;
    delete clientchannel_;
}

int Connection::fd() const                              // 返回客户端的fd。
{
    return clientsock_->fd();
}

std::string Connection::ip() const                   // 返回客户端的ip。
{
    return clientsock_->ip();
}

uint16_t Connection::port() const                  // 返回客户端的port。
{
    return clientsock_->port();
}

void Connection::closecallback()                    // TCP连接关闭（断开）的回调函数，供Channel回调。
{
    closecallback_(this);
}

void Connection::errorcallback()                    // TCP连接错误的回调函数，供Channel回调。
{
    errorcallback_(this);     // 回调TcpServer::errorconnection()。
}

// 设置关闭fd_的回调函数。
void Connection::setclosecallback(std::function<void(Connection*)> fn)    
{
    closecallback_=fn;     // 回调TcpServer::closeconnection()。
}

// 设置fd_发生了错误的回调函数。
void Connection::seterrorcallback(std::function<void(Connection*)> fn)    
{
    errorcallback_=fn;     // 回调TcpServer::errorconnection()。
}

// 设置处理报文的回调函数。
void Connection::setonmessagecallback(std::function<void(Connection*,std::string&)> fn)    
{
    onmessagecallback_=fn;       // 回调TcpServer::onmessage()。
}

// 发送数据完成后的回调函数。
void Connection::setsendcompletecallback(std::function<void(Connection*)> fn)    
{
    sendcompletecallback_=fn;
}

// 处理对端发送过来的消息。
void Connection::onmessage()
{
    char buffer[1024];
    while (true)             // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {    
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer));
        if (nread > 0)      // 成功的读取到了数据。
        {
            inputbuffer_.append(buffer,nread);      // 把读取的数据追加到接收缓冲区中。
        } 
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
        {  
            continue;
        } 
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
        {
            while (true)             // 从接收缓冲区中拆分出客户端的请求消息。
            {
                //////////////////////////////////////////////////////////////
                // 可以把以下代码封装在Buffer类中，还可以支持固定长度、指定报文长度和分隔符等多种格式。
                int len;
                memcpy(&len,inputbuffer_.data(),4);     // 从inputbuffer中获取报文头部。
                // 如果inputbuffer中的数据量小于报文头部，说明inputbuffer中的报文内容不完整。
                if (inputbuffer_.size()<len+4) break;

                std::string message(inputbuffer_.data()+4,len);   // 从inputbuffer中获取一个报文。
                inputbuffer_.erase(0,len+4);                                 // 从inputbuffer中删除刚才已获取的报文。
                //////////////////////////////////////////////////////////////

                printf("message (eventfd=%d):%s\n",fd(),message.c_str());

                onmessagecallback_(this,message);       // 回调TcpServer::onmessage()处理客户端的请求消息。
            }
            break;
        } 
        else if (nread == 0)  // 客户端连接已断开。
        {  
            closecallback();                     // 回调TcpServer::closecallback()。
            break;
        }
    }
}

// 发送数据。
void Connection::send(const char *data,size_t size)        
{
    outputbuffer_.appendwithhead(data,size);    // 把需要发送的数据保存到Connection的发送缓冲区中。
    clientchannel_->enablewriting();    // 注册写事件。
}

// 处理写事件的回调函数，供Channel回调。
void Connection::writecallback()                   
{
    int writen=::send(fd(),outputbuffer_.data(),outputbuffer_.size(),0);    // 尝试把outputbuffer_中的数据全部发送出去。
    if (writen>0) outputbuffer_.erase(0,writen);                                        // 从outputbuffer_中删除已成功发送的字节数。

    // 如果发送缓冲区中没有数据了，表示数据已发送完成，不再关注写事件。
    if (outputbuffer_.size()==0) 
    {
        clientchannel_->disablewriting();        
        sendcompletecallback_(this);
    }
}