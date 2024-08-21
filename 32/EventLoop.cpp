#include "EventLoop.h"

// 在构造函数中创建Epoll对象ep_。
EventLoop::EventLoop()
                  :ep_(new Epoll),wakeupfd_(eventfd(0,EFD_NONBLOCK)),wakechannel_(new Channel(this,wakeupfd_))
{
    wakechannel_->setreadcallback(std::bind(&EventLoop::handlewakeup,this));
    wakechannel_->enablereading();
}

// 在析构函数中销毁ep_。
EventLoop::~EventLoop()
{
    // delete ep_;
}

// 运行事件循环。
void EventLoop::run()                      
{
    // printf("EventLoop::run() thread is %d.\n",syscall(SYS_gettid));
    threadid_=syscall(SYS_gettid);    // 获取事件循环所在线程的id。

    while (true)        // 事件循环。
    {
       std::vector<Channel *> channels=ep_->loop(10*1000);         // 等待监视的fd有事件发生。

        // 如果channels为空，表示超时，回调TcpServer::epolltimeout()。
        if (channels.size()==0)
        {
            epolltimeoutcallback_(this);
        }
        else
        {
            for (auto &ch:channels)
            {
                ch->handleevent();        // 处理epoll_wait()返回的事件。
            }
        }
    }
}

// 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件。
void EventLoop::updatechannel(Channel *ch)                        
{
    ep_->updatechannel(ch);
}

 // 从黑树上删除channel。
 void EventLoop::removechannel(Channel *ch)                       
 {
    ep_->removechannel(ch);
 }

// 设置epoll_wait()超时的回调函数。
void EventLoop::setepolltimeoutcallback(std::function<void(EventLoop*)> fn)  
{
    epolltimeoutcallback_=fn;
}

// 判断当前线程是否为事件循环线程。
bool EventLoop::isinloopthread()   
{
    return threadid_==syscall(SYS_gettid); 
}

 // 把任务添加到队列中。
 void EventLoop::queueinloop(std::function<void()> fn)
 {
    {
        std::lock_guard<std::mutex> gd(mutex_);           // 给任务队列加锁。
        taskqueue_.push(fn);                                            // 任务入队。
    }

    wakeup();        // 唤醒事件循环。
 }

// 用eventfd唤醒事件循环线程。
 void EventLoop::wakeup()
 {
    uint64_t val=1;
    write(wakeupfd_,&val,sizeof(val));
 }

 // 事件循环线程被eventfd唤醒后执行的函数。
 void EventLoop::handlewakeup()
 {
    printf("handlewakeup() thread id is %d.\n",syscall(SYS_gettid));

    uint64_t val;
    read(wakeupfd_,&val,sizeof(val));       // 从eventfd中读取出数据，如果不读取，eventfd的读事件会一直触发。

    std::function<void()> fn;

    std::lock_guard<std::mutex> gd(mutex_);           // 给任务队列加锁。

    // 执行队列中全部的发送任务。
    while (taskqueue_.size()>0)
    {
        fn=std::move(taskqueue_.front());    // 出队一个元素。
        taskqueue_.pop();                              
        fn();                                                    // 执行任务。
    }
 }