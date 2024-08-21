#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port)
{
    acceptor_=new Acceptor(&loop_,ip,port);
    acceptor_->setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));
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

    printf ("new connection(fd=%d,ip=%s,port=%d) ok.\n",conn->fd(),conn->ip().c_str(),conn->port());

    conns_[conn->fd()]=conn;            // 把conn存放map容器中。
}
