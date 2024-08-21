#include "Connection.h"

Connection::Connection(EventLoop* loop,std::unique_ptr<Socket> clientsock)
                    :loop_(loop),clientsock_(std::move(clientsock_)),disconnect_(false),clientchannel_(new Channel(loop_,clientsock_->fd()))
{
    clientchannel_->setreadcallback(std::bind(&Connection::onmessage,this));
    clientchannel_->setclosecallback(std::bind(&Connection::closecallback,this));
    clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback,this));
    clientchannel_->setwritecallback(std::bind(&Connection::writecallback,this));
    clientchannel_->useet();
    clientchannel_->enablereading();
}

Connection::~Connection()
{
    // printf("conn已析构。\n");
}

int Connection::fd() const
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

void Connection::closecallback(){
    disconnect_=true;
    clientchannel_->remove();
    closecallback_(shared_from_this());
}

void Connection::errorcallback()                    // TCP连接错误的回调函数，供Channel回调。
{
    disconnect_=true;
    clientchannel_->remove();                  // 从事件循环中删除Channel。
    errorcallback_(shared_from_this());     // 回调TcpServer::errorconnection()。
}

// 设置关闭fd_的回调函数。
void Connection::setclosecallback(std::function<void(spConnection)> fn)    
{
    closecallback_=fn;     // 回调TcpServer::closeconnection()。
}

// 设置fd_发生了错误的回调函数。
void Connection::seterrorcallback(std::function<void(spConnection)> fn)    
{
    errorcallback_=fn;     // 回调TcpServer::errorconnection()。
}

// 设置处理报文的回调函数。
void Connection::setonmessagecallback(std::function<void(spConnection,std::string&)> fn)    
{
    onmessagecallback_=fn;       // 回调TcpServer::onmessage()。
}

// 发送数据完成后的回调函数。
void Connection::setsendcompletecallback(std::function<void(spConnection)> fn)    
{
    sendcompletecallback_=fn;
}

void Connection::onmessage(){
    char buffer[1024];
    while(true){
        bzero(&buffer,sizeof(buffer));

        ssize_t nread = read(fd(),buffer,sizeof(buffer));
        if(nread>0){
            inputbuffer_.append(buffer,nread);
        }else if(nread==-1 && errno == EINTR){
            continue;
        }else if(nread==-1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))){
            std::string message;

            while(true){
                if(inputbuffer_.pickmessage(message)==false) break;

                lastatime_=Timestamp::now();

                onmessagecallback_(shared_from_this(),message);
            }
            break;
        }else if(nread==0){
            closecallback();
            break;
        }
    }
}

// 发送数据，不管在任何线程中，都是调用此函数发送数据。
void Connection::send(const char *data,size_t size)        
{
    if (disconnect_==true) {  printf("客户端连接已断开了，send()直接返回。\n"); return;}

    if (loop_->isinloopthread())   // 判断当前线程是否为事件循环线程（IO线程）。
    {
        // 如果当前线程是IO线程，直接调用sendinloop()发送数据。
        // printf("send() 在事件循环的线程中。\n");
        sendinloop(data,size);
    }
    else
    {
        // 如果当前线程不是IO线程，调用EventLoop::queueinloop()，把sendinloop()交给事件循环线程去执行。
        // printf("send() 不在事件循环的线程中。\n");
        loop_->queueinloop(std::bind(&Connection::sendinloop,this,data,size));
    }
}

// 发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程去执行。
void Connection::sendinloop(const char *data,size_t size)
{
    outputbuffer_.appendwithsep(data,size);    // 把需要发送的数据保存到Connection的发送缓冲区中。
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
        sendcompletecallback_(shared_from_this());
    }
}

// 判断TCP连接是否超时（空闲太久）。
bool Connection::timeout(time_t now,int val)           
{
   return now-lastatime_.toint()>val;    
}
