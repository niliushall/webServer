#ifndef _LOCKER_H_
#define _LOCKER_H_

#include <iostream>
#include <exception>
#include <pthread.h>
using namespace std;

/* 线程锁 */
class MutexLocker {
private:
    pthread_mutex_t m_mutex;
public:
    MutexLocker() {  //初始化
        if( pthread_mutex_init( &m_mutex, NULL ) ) {
            cout << "mutex init errror __ 1\n";
            throw exception();
        }
    }

    ~MutexLocker() {
        pthread_mutex_destroy( &m_mutex );
    }

    bool mutex_lock() {
        return pthread_mutex_lock( &m_mutex ) == 0;
    }

    bool mutex_unlock() {
        return pthread_mutex_unlock( &m_mutex );
    }
};


/* 条件变量 */
class Cond {
private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
public:
    Cond() {
        if( pthread_mutex_init( &m_mutex, NULL ) ) {
            throw exception();
        }
        if( pthread_cond_init( &m_cond, NULL ) ) {
            pthread_cond_destroy( &m_cond );
            throw exception();
        }
    }

    ~Cond() {
        pthread_mutex_destroy( &m_mutex );
        pthread_cond_destroy( &m_cond );
    }

    // 等待条件变量，cond与mutex搭配使用，避免造成共享数据的混乱
    bool wait() {
        pthread_mutex_lock( &m_mutex );
        int ret = pthread_cond_wait( &m_cond, &m_mutex );
        pthread_mutex_unlock( &m_mutex );
        return ret == 0;
    }

    // 唤醒等待该条件变量的某个线程
    bool signal() {
        return pthread_cond_signal( &m_cond ) == 0;
    }

    // 唤醒所有等待该条件变量的线程
    bool broadcast() {
        return pthread_cond_broadcast( &m_cond ) == 0;
    }
};

#endif