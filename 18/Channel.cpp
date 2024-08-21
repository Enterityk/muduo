#include "Channel.h"

Channel::Channel(EventLoop* loop,int fd):loop_(loop),fd_(fd)      // 构造函数。
{

}

Channel::~Channel()                           // 析构函数。 
{
    // 在析构函数中，不要销毁loop_，也不能关闭fd_，因为这两个东西不属于Channel类，Channel类只是需要它们，使用它们而已。
}

int Channel::fd()                                            // 返回fd_成员。
{
    return fd_;
}

void Channel::useet()                                    // 采用边缘触发。
{
    events_=events_|EPOLLET;
}

void Channel::enablereading()                     // 让epoll_wait()监视fd_的读事件。
{
    events_|=EPOLLIN;
    loop_->updatechannel(this);
}

void Channel::setinepoll()                           // 把inepoll_成员的值设置为true。
{
    inepoll_=true;
}

void Channel::setrevents(uint32_t ev)         // 设置revents_成员的值为参数ev。
{
    revents_=ev;
}

bool Channel::inpoll()                                  // 返回inepoll_成员。
{
    return inepoll_;
}

uint32_t Channel::events()                           // 返回events_成员。
{
    return events_;
}

uint32_t Channel::revents()                          // 返回revents_成员。
{
    return revents_;
} 

// 事件处理函数，epoll_wait()返回的时候，执行它。
void Channel::handleevent()
{
    if (revents_ & EPOLLRDHUP)                     // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
    {
        // printf("client(eventfd=%d) disconnected.\n",fd_);
        // close(fd_);            // 关闭客户端的fd。
        closecallback_();      // 回调Connection::closecallback()。
    }                               
    else if (revents_ & (EPOLLIN|EPOLLPRI))   // 接收缓冲区中有数据可以读。
    {
        readcallback_();   // 如果是acceptchannel，将回调Acceptor::newconnection()，如果是clientchannel，将回调Channel::onmessage()。
    }
    else if (revents_ & EPOLLOUT)                  // 有数据需要写，暂时没有代码，以后再说。
    {
    }
    else                                                           // 其它事件，都视为错误。
    {
        // printf("client(eventfd=%d) error.\n",fd_);
        // close(fd_);            // 关闭客户端的fd。
        errorcallback_();       // 回调Connection::errorcallback()。
    }
}

// 处理对端发送过来的消息。
void Channel::onmessage()
{
    char buffer[1024];
    while (true)             // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {    
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd_, buffer, sizeof(buffer));
        if (nread > 0)      // 成功的读取到了数据。
        {
            // 把接收到的报文内容原封不动的发回去。
            printf("recv(eventfd=%d):%s\n",fd_,buffer);
            send(fd_,buffer,strlen(buffer),0);
        } 
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
        {  
            continue;
        } 
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
        {
            break;
        } 
        else if (nread == 0)  // 客户端连接已断开。
        {  
            // printf("client(eventfd=%d) disconnected.\n",fd_);
            // close(fd_);            // 关闭客户端的fd。
            closecallback_();
            break;
        }
    }
}

 // 设置fd_读事件的回调函数。
 void Channel::setreadcallback(std::function<void()> fn)    
 {
    readcallback_=fn;
 }

 // 设置关闭fd_的回调函数。
 void Channel::setclosecallback(std::function<void()> fn)    
 {
    closecallback_=fn;
 }

 // 设置fd_发生了错误的回调函数。
 void Channel::seterrorcallback(std::function<void()> fn)    
 {
    errorcallback_=fn;
 }