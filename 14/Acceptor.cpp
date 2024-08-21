#include "Acceptor.h"

Acceptor::Acceptor(EventLoop *loop,const std::string &ip,const uint16_t port):loop_(loop)
{
    servsock_=new Socket(createnonblocking());   
    InetAddress servaddr(ip,port);             // 服务端的地址和协议。
    servsock_->setreuseaddr(true);
    servsock_->settcpnodelay(true);
    servsock_->setreuseport(true);
    servsock_->setkeepalive(true);
    servsock_->bind(servaddr);
    servsock_->listen();

    acceptchannel_=new Channel(loop_,servsock_->fd());       
    // acceptchannel_->setreadcallback(std::bind(&Channel::newconnection,acceptchannel_,servsock_));
    acceptchannel_->setreadcallback(std::bind(&Acceptor::newconnection,this));
    acceptchannel_->enablereading();       // 让epoll_wait()监视servchannel的读事件。 
}

Acceptor::~Acceptor()
{
    delete servsock_;
    delete acceptchannel_;
}

#include "Connection.h"

// 处理新客户端连接请求。
void Acceptor::newconnection()    
{
    InetAddress clientaddr;             // 客户端的地址和协议。
    // 注意，clientsock只能new出来，不能在栈上，否则析构函数会关闭fd。
    // 还有，这里new出来的对象没有释放，将在Connection类的析构函数中释放。
    Socket *clientsock=new Socket(servsock_->accept(clientaddr));

    printf ("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientsock->fd(),clientaddr.ip(),clientaddr.port());

    /*
    // 为新客户端连接准备读事件，并添加到epoll中。
    Channel *clientchannel=new Channel(loop_,clientsock->fd());   // 这里new出来的对象没有释放，这个问题以后再解决。
    clientchannel->setreadcallback(std::bind(&Channel::onmessage,clientchannel));
    clientchannel->useet();                 // 客户端连上来的fd采用边缘触发。
    clientchannel->enablereading();   // 让epoll_wait()监视clientchannel的读事件
    */
    Connection *conn=new Connection(loop_,clientsock);   // 这里new出来的对象没有释放，这个问题以后再解决。
}
