#pragma once
#include <functional>
#include "Epoll.h"
#include <memory>
#include <unistd.h>
#include <queue>
#include <mutex>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>      // 定时器需要包含这个头文件。

class Channel;
class Epoll;

// 事件循环类。
class EventLoop
{
private:
    std::unique_ptr<Epoll> ep_;                                   // 每个事件循环只有一个Epoll。
    std::function<void(EventLoop*)> epolltimeoutcallback_;         // epoll_wait()超时的回调函数。
    pid_t threadid_;                                                       // 事件循环所在线程的id。
    std::queue<std::function<void()>> taskqueue_;    // 事件循环线程被eventfd唤醒后执行的任务队列。
    std::mutex mutex_;                                                  // 任务队列同步的互斥锁。
    int wakeupfd_;                                                         // 用于唤醒事件循环线程的eventfd。
    std::unique_ptr<Channel> wakechannel_;              // eventfd的Channel。
    int timerfd_;                                                             // 定时器的fd。
    std::unique_ptr<Channel> timerchannel_;              // 定时器的Channel。
    bool mainloop_;                                                       // true-是主事件循环，false-是从事件循环。

public:
    EventLoop(bool mainloop);                   // 在构造函数中创建Epoll对象ep_。
    ~EventLoop();               

    void run();                      // 运行事件循环。

    void updatechannel(Channel *ch);                        // 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件。
    void removechannel(Channel *ch);                       // 从黑树上删除channel。
    void setepolltimeoutcallback(std::function<void(EventLoop*)> fn);  // 设置epoll_wait()超时的回调函数。

    bool isinloopthread();   // 判断当前线程是否为事件循环线程。

    void queueinloop(std::function<void()> fn);          // 把任务添加到队列中。
    void wakeup();                                                        // 用eventfd唤醒事件循环线程。
    void handlewakeup();                                             // 事件循环线程被eventfd唤醒后执行的函数。

    void handletimer();                                                 // 闹钟响时执行的函数。
};