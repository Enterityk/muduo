/*
 * 程序名：tcpepoll.cpp，此程序用于演示采用epoll模型实现网络通讯的服务端。
 * 作者：吴从周
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>          
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>      // TCP_NODELAY需要包含这个头文件。
#include "InetAddress.h"
#include "Socket.h"
#include "Epoll.h"

int main(int argc,char *argv[])
{
    if (argc != 3) 
    { 
        printf("usage: ./tcpepoll ip port\n"); 
        printf("example: ./tcpepoll 192.168.150.128 5085\n\n"); 
        return -1; 
    }

    Socket servsock(createnonblocking());//创建一个非阻塞的服务端的socket
    InetAddress servaddr(argv[1],atoi(argv[2]));// 服务端的地址和协议。

    servsock.setreuseaddr(true);
    servsock.settcpnodelay(true);
    servsock.setreuseport(true);
    servsock.setkeepalive(true);
    servsock.bind(servaddr);//绑定地址
    servsock.listen();//监听

    Epoll ep;
    Channel *servchannel=new Channel(&ep,servsock.fd());//传入epoll句柄和socket_fd       // 这里new出来的对象没有释放，这个问题以后再解决。
    //使用std::bind绑定成员函数及其对象  还有输入参数
    servchannel->setreadcallback(std::bind(&Channel::newconnection,servchannel,&servsock));
    servchannel->enablereading();//将Channel加入epoll event对应的ptr中       // 让epoll_wait()监视servchannel的读事件。

    while (true)        // 事件循环。
    {
       std::vector<Channel *> channels=ep.loop();         // 等待监视的fd有事件发生。

        for (auto &ch:channels)
        {
            ch->handleevent();        // 处理epoll_wait()返回的事件。
        }
    }

  return 0;
}