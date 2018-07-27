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
using namespace std;

const int buffer_size = 1024;

// 封装任务执行的类
class Task {
private:
    int accp_fd;  // 存储accept的返回值

public:
    Task() {}
    Task(int fd) : accp_fd(fd) {}
    ~Task() { close( accp_fd ); }

    void doit();  // 执行任务

    // 构造HTTP首部
    const string construct_header( const int num, const string & info,
                                        const string & type );

    // 发送文件
    int send_file( const string & filename, const string & type, 
                        const int num = 200, const string & info = "OK" );
    int deal_get( const string & uri );  // 处理GET请求
    int deal_post( const string & uri, char *buf );  // 处理POST请求
    int get_size( const string & filename );  // 获取文件大小
};

#endif