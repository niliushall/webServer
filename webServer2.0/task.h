#ifndef _TASK_H_
#define _TASK_H_

#include "threadPool.h"
#include <sstream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <errno.h>
using namespace std;

const int buffer_size = 1024;

void removefd( int epollfd, int fd );

// 封装任务执行的类
class Task {
private:
    int accp_fd;  // 存储accept的返回值
    int epoll_fd;

public:
    Task() {}
    Task(int fd, int epoll) : accp_fd(fd), epoll_fd(epoll) {}
    ~Task() { removefd( epoll_fd, accp_fd ); }
    // Task(int fd, int epoll) : accp_fd(fd), epoll_fd(epoll) { cout << accp_fd << " start\n"; }
    // ~Task() { removefd( epoll_fd, accp_fd ); cout << accp_fd << " close\n"; }


    void doit();  // 执行任务
    // 构造HTTP首部
    // void construct_header( char *response, const int num, const char * info,
    //                                     const char * type );

    // 发送文件
    int send_file( const string & filename, const char *type, 
                        int start, const int num = 200, const char *info = "OK" );
    int deal_get( const string & uri, int start = 0 );  // 处理GET请求
    int deal_post( const string & uri, char *buf );  // 处理POST请求
    int get_size( const string & filename );  // 获取文件大小
};

#endif
