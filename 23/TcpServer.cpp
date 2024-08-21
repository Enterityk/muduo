#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port)
{
    acceptor_=new Acceptor(&loop_,ip,port);
    acceptor_->setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));
    loop_.setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
}

TcpServer::~TcpServer()
{
    delete acceptor_;

    // 释放全部的Connection对象。
    for (auto &aa:conns_)
    {
        delete aa.second;
    }
}

// 运行事件循环。
void TcpServer::start()          
{
    loop_.run();
}

// 处理新客户端连接请求。
void TcpServer::newconnection(Socket* clientsock)
{
    Connection *conn=new Connection(&loop_,clientsock);   
    conn->setclosecallback(std::bind(&TcpServer::closeconnection,this,std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection,this,std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage,this,std::placeholders::_1,std::placeholders::_2));
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete,this,std::placeholders::_1));

    // printf ("new connection(fd=%d,ip=%s,port=%d) ok.\n",conn->fd(),conn->ip().c_str(),conn->port());

    conns_[conn->fd()]=conn;            // 把conn存放map容器中。

    if (newconnectioncb_) newconnectioncb_(conn);             // 回调EchoServer::HandleNewConnection()。
}

 // 关闭客户端的连接，在Connection类中回调此函数。 
 void TcpServer::closeconnection(Connection *conn)
 {
    if (closeconnectioncb_) closeconnectioncb_(conn);       // 回调EchoServer::HandleClose()。

    // printf("client(eventfd=%d) disconnected.\n",conn->fd());
    conns_.erase(conn->fd());        // 从map中删除conn。
    delete conn;
 }

// 客户端的连接错误，在Connection类中回调此函数。
void TcpServer::errorconnection(Connection *conn)
{
    if (errorconnectioncb_) errorconnectioncb_(conn);     // 回调EchoServer::HandleError()。

    // printf("client(eventfd=%d) error.\n",conn->fd());
    conns_.erase(conn->fd());      // 从map中删除conn。
    delete conn;
}

// 处理客户端的请求报文，在Connection类中回调此函数。
void TcpServer::onmessage(Connection *conn,std::string message)
{
    /*
    // 在这里，将经过若干步骤的运算。
    message="reply:"+message;          // 回显业务。
                
    int len=message.size();                   // 计算回应报文的大小。
    std::string tmpbuf((char*)&len,4);  // 把报文头部填充到回应报文中。
    tmpbuf.append(message);             // 把报文内容填充到回应报文中。
                
    conn->send(tmpbuf.data(),tmpbuf.size());   // 把临时缓冲区中的数据发送出去。
    */
    if (onmessagecb_) onmessagecb_(conn,message);     // 回调EchoServer::HandleMessage()。
}

// 数据发送完成后，在Connection类中回调此函数。
void TcpServer::sendcomplete(Connection *conn)     
{
    // printf("send complete.\n");

    if (sendcompletecb_) sendcompletecb_(conn);     // 回调EchoServer::HandleSendComplete()。
}

// epoll_wait()超时，在EventLoop类中回调此函数。
void TcpServer::epolltimeout(EventLoop *loop)         
{
    // printf("epoll_wait() timeout.\n");

    if (timeoutcb_)  timeoutcb_(loop);           // 回调EchoServer::HandleTimeOut()。
}

void TcpServer::setnewconnectioncb(std::function<void(Connection*)> fn)
{
    newconnectioncb_=fn;
}

void TcpServer::setcloseconnectioncb(std::function<void(Connection*)> fn)
{
    closeconnectioncb_=fn;
}

void TcpServer::seterrorconnectioncb(std::function<void(Connection*)> fn)
{
    errorconnectioncb_=fn;
}

void TcpServer::setonmessagecb(std::function<void(Connection*,std::string &message)> fn)
{
    onmessagecb_=fn;
}

void TcpServer::setsendcompletecb(std::function<void(Connection*)> fn)
{
    sendcompletecb_=fn;
}

void TcpServer::settimeoutcb(std::function<void(EventLoop*)> fn)
{
    timeoutcb_=fn;
}
