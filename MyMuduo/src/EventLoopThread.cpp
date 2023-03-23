#include <memory>
#include "EventLoopThread.hpp"
#include "EventLoop.hpp"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const string &name)
    : loop_(nullptr), exiting_(false), thread_(bind(&EventLoopThread::thread_function, this), name), thread_mutex_(), condition_(), callback_function_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::start_loop()
{
    thread_.start(); //启动线程

    EventLoop *loop = nullptr;
    {
        unique_lock<mutex> lock(thread_mutex_);  // 这里看来是有讲究的
        while (loop_ == nullptr)
        {
            condition_.wait(lock);  // 这里会阻塞, 需要等 thread function 设置了loop_ 变量之后,才可以继续往下走
        }
        loop = loop_;
    }

    return loop;
}

//启动的线程中执行以下方法
// 在Thread::start()中执行, 说明是先执行  thread_function, 后执行 start_loop
void EventLoopThread::thread_function()
{
    EventLoop loop; //创建一个独立的EventLoop，和上面的线程是一一对应 one loop per thread

    if (callback_function_)  // 尝试执行线程初始化函数 
    {
        callback_function_(&loop);
    }

    {
        unique_lock<mutex> lock(thread_mutex_);
        loop_ = &loop;
        condition_.notify_one();  // 
    }
    // 一个线程执行一个loop, one thread one loop
    loop.loop(); //开启事件循环

    //结束事件循环
    unique_lock<mutex> lock(thread_mutex_);
    loop_ = nullptr;
}