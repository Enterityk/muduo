#include "TcpServer.h"

/*
class TcpServer
{
private:
    EventLoop loop_;         // 一个TcpServer可以有多个事件循环，现在是单线程，暂时只用一个事件循环。
public:
    TcpServer(const std::string &ip,const uint16_t port);
    ~TcpServer();
};*/

TcpServer::TcpServer(const std::string &ip,const uint16_t port)
{
    Socket *servsock=new Socket(createnonblocking());   // 这里new出来的对象没有释放，以后再说。
    InetAddress servaddr(ip,port);             // 服务端的地址和协议。
    servsock->setreuseaddr(true);
    servsock->settcpnodelay(true);
    servsock->setreuseport(true);
    servsock->setkeepalive(true);
    servsock->bind(servaddr);
    servsock->listen();

    Channel *servchannel=new Channel(&loop_,servsock->fd());       // 这里new出来的对象没有释放，这个问题以后再解决。
    servchannel->setreadcallback(std::bind(&Channel::newconnection,servchannel,servsock));
    servchannel->enablereading();       // 让epoll_wait()监视servchannel的读事件。
}

TcpServer::~TcpServer()
{

}

// 运行事件循环。
void TcpServer::start()          
{
    loop_.run();
}