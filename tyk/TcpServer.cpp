#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port,int threadnum)
                ::threadnum_(threadnum),mainloop_(new EventLoop(true)),
                acceptor_(mainloop_.get(),ip,port),threadpool_(threadnum_,"IO")
{
    mainloop_->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));

    acceptor_.setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));

    for(int ii=0;ii<threadnum_;ii++){
        subloops_.emplace_back(new EventLoop(false,5,10));
        subloops_[ii]->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout.this,std::placeholders::_1));
        subloops_[ii]->settimercallback(std::bind(&TcpServer::removeconn,this,std::placeholders::_1));
        threadpool_.addtask(std::bind(&EventLoop::run,subloops_[ii].get()));
    }
}

TcpServer::~TcpServer(){}

void TcpServer::start(){
    mainloop_->run();
}

void TcpServer::stop(){
    // 停止主事件循环。
    mainloop_->stop();
    printf("主事件循环已停止。\n");

    // 停止从事件循环。
    for (int ii=0;ii<threadnum_;ii++)
    {
        subloops_[ii]->stop();
    }
    printf("从事件循环已停止。\n");

    // 停止IO线程。
    threadpool_.stop();
    printf("IO线程池停止。\n");
}

void TcpServer::newconnection(std::unique_ptr<Socket> clientsock){
    //新建的连接分配给从事件循环
    spConnection conn(new Connection(subloops_[clientsock->fd()%threadnum_].get(),std::move(clientsock)));
    conn->setclosecallback(std::bind(&TcpServer::closeconnection,this,std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection,this,std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage,this,std::placeholders::_1,std::placeholders::_2));
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete,this,std::placeholders::_1));

    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_[conn->fd()]=conn;  //把conn存放到TcpServer的map容器中
    }
    subloops_[conn->fd()%threadnum_]->newconnection(conn);

    if(newconnectioncb_) newconnectioncb_(conn);
}

void TcpServer::closeconnection(spConnection conn){
    if(closeconnectioncb_) closeconnectioncb_(conn);

    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->fd());
    }
}

void TcpServer::errorconnection(spConnection conn){
    if(errorconnectioncb_) errorconnectioncb_(conn);

    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->fd());
    }
}

void TcpServer::onmessage(spConnection conn,std::string& message){
    if(onmessagecb_) onmessagecb_(conn,message);
}

void TcpServer::sendcomplete(spConnection conn){
    if(sendcompletecb_) sendcompletecb_(conn);
}

void TcpServer::epolltimeout(EventLoop *loop){
    if(timeoutcb_) timeoutcb_(loop);
}

void TcpServer::setnewconnectioncb(std::function<void(spConnection)> fn){
    newconnectioncb_=fn;
}


void TcpServer::setcloseconnectioncb(std::function<void(spConnection)> fn)
{
    closeconnectioncb_=fn;
}

void TcpServer::seterrorconnectioncb(std::function<void(spConnection)> fn)
{
    errorconnectioncb_=fn;
}

void TcpServer::setonmessagecb(std::function<void(spConnection,std::string &message)> fn)
{
    onmessagecb_=fn;
}

void TcpServer::setsendcompletecb(std::function<void(spConnection)> fn)
{
    sendcompletecb_=fn;
}

void TcpServer::settimeoutcb(std::function<void(EventLoop*)> fn)
{
    timeoutcb_=fn;
}

// 删除conns_中的Connection对象，在EventLoop::handletimer()中将回调此函数。
void TcpServer::removeconn(int fd)                 
{
    {
         std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(fd);          // 从map中删除conn。
    }
}
