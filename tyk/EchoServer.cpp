#include "EchoServer.h"

EchoServer::EchoServer(const std::string &ip,const uint16_t port,int subthreadnum,int workthreadnum)
                    :tcpserver_(ip,port,subthreadnum),threadpool_(workthreadnum,"WORKS")
{
    tcpserver_.setnewconnectioncb(std::bind(&EchoServer::HandleNewConnection,this,std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&EchoServer::HandleClose,this,std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&EchoServer::HandleError,this,std::placeholders::_1));
    tcpserver_.setonmessagecb(std::bind(&EchoServer::HandleMessage,this,std::placeholders::_1,std::placeholders::_2));
    tcpserver_.setsendcompletecb(std::bind(&EchoServer::HandleSendComplete,this,std::placeholders::_1));
}

EchoServer::~EchoServer(){}

void EchoServer::Start(){
    tcpserver_.start();
}


 // 停止服务。
void EchoServer::Stop()
{
    // 停止工作线程。
    threadpool_.stop();
    printf("工作线程已停止。\n");

    // 停止IO线程（事件循环）。
    tcpserver_.stop();
}

void EchoServer::HandleNewConnection(spConnection conn){
    printf ("%s new connection(fd=%d,ip=%s,port=%d) ok.\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().c_str(),conn->port());
}

void EchoServer::HandleClose(spConnection conn){
    printf ("%s connection closed(fd=%d,ip=%s,port=%d) ok.\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().c_str(),conn->port());
}


// 客户端的连接错误，在TcpServer类中回调此函数。
void EchoServer::HandleError(spConnection conn)  
{
    // std::cout << "EchoServer conn error." << std::endl;

    // 根据业务的需求，在这里可以增加其它的代码。
}

void EchoServer::HandleMessage(spConnection conn,std::string& message){
    if(threadpool_.size()==0){
        OnMessage(conn,message);
    }else{
        threadpool_.addtask(std::bind(&EchoServer::OnMessage,this,conn,message));
    }
}

void EchoServer::OnMessage(spConnection conn,std::string& message){
    message="replay"+message;

    conn->send(message.data(),message.size());
}


// 数据发送完成后，在TcpServer类中回调此函数。
void EchoServer::HandleSendComplete(spConnection conn)     
{
    // std::cout << "Message send complete." << std::endl;

    // 根据业务的需求，在这里可以增加其它的代码。
}
