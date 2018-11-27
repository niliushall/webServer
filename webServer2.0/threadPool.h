#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "locker.h"
#include <queue>
using namespace std;

template< typename T >
class ThreadPool {
private:
    int thread_number;  // 线程池的线程数
    pthread_t *threads;  // 线程数组
    queue<T *> task_queue;  // 任务队列
    MutexLocker queue_mutex_locker;  // 任务队列的互斥锁
    Cond queue_cond_locker;  // 任务队列的条件变量
    bool m_stop;  // 是否结束线程
public:
    ThreadPool( int thread_num = 20 );
    ~ThreadPool();
    bool append( T *task );  // 向任务队列添加任务
private:
    static void *worker( void * );  // 工作线程
    void run();  // 线程池中线程开始运行的函数
    T *getTask();  // 从任务队列中获取队首的任务
};

template< typename T >
ThreadPool<T>::ThreadPool( int thread_num ) :thread_number(thread_num), 
                            threads(NULL), m_stop(false) {
    if( thread_number < 0 ) {
        cout << "thread_number < 0\n";
        throw exception();
    }
    
    // 创建数组存放线程号
    threads = new pthread_t[ thread_number ];
    if( !threads ) {
        cout << "threads is NULL\n";
        throw exception();
    }

    // 创建规定数量的线程
    for( int i = 0; i < thread_number; i++ ) {
        // 由于pthread_create第三个参数必须指向静态函数，要使用类成员函数和变量，只能通过：
        // 1) 类的静态对象
        // 2) 将类的对象作为参数传给静态函数
        // 这里通过第二种方法实现
        if( pthread_create( &threads[i], NULL, worker, this ) ) {  // 成功返回0
            delete[] threads;  // 创建失败则释放所有已分配的空间
            cout << "pthread_create error\n";
            throw exception();
        }

        // 将线程进行脱离，线程运行完后自动回收，避免使用主线程进行join等待其结束
        if( pthread_detach( threads[i] ) ) {
            delete[] threads;
            cout << "pthread_detach error\n";
            throw exception();
        }
    }
}

// 析构函数中，将m_stop置true，此时将阻塞中的所有线程唤醒
// 由于 !m_stop 为false，线程会退出循环，线程结束被回收（ 详见函数run() ）
// 若不唤醒线程，则在程序退出后，线程非正常结束，同时会导致
template< typename T >
ThreadPool<T>::~ThreadPool() {
    delete[] threads;
    m_stop = true;
    queue_cond_locker.broadcast();
}

/* 添加任务时需要先上锁，并判断队列是否为空 */
template< typename T >
bool ThreadPool<T>::append( T *task ) {
    queue_mutex_locker.mutex_lock();
    bool need_signal = task_queue.empty();  // 记录添加任务之前队列是否为空
    task_queue.push( task );
    queue_mutex_locker.mutex_unlock();

    // 如果添加任务之前队列为空，即所有线程都在wait，所以需要唤醒某个线程
    if( need_signal ) {
        queue_cond_locker.signal();
    }

    return true;
}

// 线程函数，调用run()来使线程开始工作
template< typename T >
void * ThreadPool<T>::worker( void *arg ) {
    ThreadPool *pool = ( ThreadPool * )arg;
    pool->run();
    return pool;
}

// 获取处于队首的任务，获取时需要加锁，避免发生错误
// 若队列为空，则返回NULL，该线程成为等待状态（详见函数run()）
template< typename T >
T* ThreadPool<T>::getTask() {
    T *task = NULL;
    queue_mutex_locker.mutex_lock();
    if( !task_queue.empty() ) {
        task = task_queue.front();
        task_queue.pop();
    }
    queue_mutex_locker.mutex_unlock();

    return task;
}

template< typename T >
void ThreadPool<T>::run() {
    while( !m_stop ) {  // 当线程池没有结束时，线程循环获取任务进行执行
        T *task = getTask();
        if( !task ) {
            queue_cond_locker.wait();  // 队列为空，线程开始等待
        } else {
// printf("a thread start work\n");
            task->doit();  // 开始执行任务
            delete task;  //task指向的对象在WebServer中new出来，因此需要手动delete
        }
    }
}

#endif
