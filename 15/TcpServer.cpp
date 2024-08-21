#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port)
{
    acceptor_=new Acceptor(&loop_,ip,port);
    acceptor_->setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));
}

TcpServer::~TcpServer()
{
    delete acceptor_;
}

// 运行事件循环。
void TcpServer::start()          
{
    loop_.run();
}

// 处理新客户端连接请求。
void TcpServer::newconnection(Socket* clientsock)
{
    printf ("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientsock->fd(),clientaddr.ip(),clientaddr.port());
    Connection *conn=new Connection(&loop_,clientsock);   // 这里new出来的对象没有释放，这个问题以后再解决。
}
