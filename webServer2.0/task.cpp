#include "task.h"

void removefd( int epollfd, int fd ) {
    epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
    close( fd );
}

void reset_oneshot( int epoll_fd, int fd ) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl( epoll_fd, EPOLL_CTL_MOD, fd, &event );
}

void Task::doit() {
    char buf[ buffer_size ] = {0};

    while( int r = recv( accp_fd, buf, 1024, 0 ) ) {
        if( !r ) {
            cout << " browser exit.\n";
            break;
        } else if( r < 0 ) {  // 如果接收出错则继续接收数据
            continue;
        }

        int start = 0;
        char method[5], uri[100], version[10];
        sscanf( buf, "%s %s %s", method, uri, version );

        if( char *tmp = strstr( buf, "Range:" ) ) {
            tmp += 13;
            sscanf( tmp, "%d", &start );
        }

        if( !strcmp( method, "GET" ) ) {  // 为GET
            deal_get( uri, start );
        } else if( !strcmp( method, "POST" ) ) {  // 为POST
            deal_post( uri, buf );
        } else {
            const char *header = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/plain;charset=utf-8\r\n\r\n";
            send( accp_fd, header, strlen(header), 0 );
        }
        break;  // 只要处理完就退出循环，避免浏览器一直处于pending状态
    }
    // close( accp_fd );  // 任务完成直接close，不能在析构函数close(如果不delete task的话，
                          // 不delete task不够条用析构函数)
}

int Task::deal_get( const string & uri, int start ) {
    string filename = uri.substr(1);

    if( uri == "/" || uri == "/index.html" ) {
        send_file( "index.html", "text/html", start );
    } else if( uri.find( ".jpg" ) != string::npos || uri.find( ".png" ) != string::npos ) {
        send_file( filename, "image/jpg", start );
    } else if( uri.find( ".html" ) != string::npos ) {
        send_file( filename, "text/html", start );
    } else if( uri.find( ".ico" ) != string::npos ) {
        send_file( filename, "image/x-icon", start );
    } else if( uri.find( ".js" ) != string::npos ) {
        send_file( filename, "yexy/javascript", start );
    } else if( uri.find( ".css" ) != string::npos ) {
        send_file( filename, "text/css", start );
    } else if( uri.find( ".mp3" ) != string::npos ) {
        send_file( filename, "audio/mp3", start );
    } else if( uri.find( ".mp4" ) != string::npos ) {
        send_file( filename, "audio/mp4", start );
    } else {
        send_file( filename, "text/plain", start );
    }
}

int Task::deal_post( const string & uri, char *buf ) {
    string filename = uri.substr(1);
    if( uri.find( "adder" ) != string::npos ) {  //使用CGI服务器，进行加法运算
        char *tmp = buf;
        int len, a, b;
        char *l = strstr( tmp, "Content-Length:" );  // 获取请求报文主体大小
        sscanf( l, "Content-Length: %d", &len );
        len = strlen( tmp ) - len;
        tmp += len;
        sscanf( tmp, "a=%d&b=%d", &a, &b );
        sprintf(tmp, "%d+%d,%d", a, b, accp_fd);  // tmp存储发送到CGI服务器的参数

        // fork产生子进程，执行CGI服务器进行计算（webServer一眼只进行解析、发送数据，不进行相关计算）
        if( fork() == 0 ) {
            // dup2( accp_fd, STDOUT_FILENO );
            execl( filename.c_str(), tmp, NULL );
        }
        wait( NULL );  // 等待子进程结束
    } else {
        send_file( "html/404.html", "text/html", 0, 404, "Not Found" );
    }
}

// type对应response的Content-Type，num为状态码，info为状态描述
int Task::send_file( const string & filename, const char *type, 
                            int start, const int num, const char *info ) {
    struct stat filestat;
    int ret = stat( filename.c_str(), &filestat );
    if( ret < 0 || !S_ISREG( filestat.st_mode ) ) {  // 打开文件出错或没有该文件
        cout << "file not found : " << filename << endl;
        send_file( "html/404.html", "text/html", 0, 404, "Not Found" );
        return -1;
    }

    char header[200];
    sprintf( header, "HTTP/1.1 %d %s\r\nServer: niliushall\r\nContent-Length: %d\r\nContent-Type: %s;charset:utf-8\r\n\r\n", num, info, int(filestat.st_size - start), type );

    // send第二个参数只能是c类型字符串，不能使用string
    send( accp_fd, header, strlen(header), 0 );

    int fd = open( filename.c_str(), O_RDONLY );
    int sum = start;

    while( sum < filestat.st_size ) {
        off_t t = sum;

        int r = sendfile( accp_fd, fd, &t, filestat.st_size );

        if( r < 0 ) {
            printf("errno = %d, r = %d\n", errno, r);
            // perror("sendfile : ");
            if( errno == EAGAIN ) {
                printf("errno is EAGAIN\n");
                // reset_oneshot( epoll_fd, accp_fd );
                continue;
            } else {
                perror( "sendfile " );
                close( fd );
                break;
            }
        } else {
            sum += r;
        }
    }
    close( fd );
// printf( "sendfile finish, %d\n", accp_fd );
    return 0;
}

int Task::get_size( const string & filename ) {
    struct stat filestat;
    int ret = stat( filename.c_str(), &filestat );
    if( ret < 0 ) {
        cout << "file not found : " << filename << endl;
        return 0;
    }
    return filestat.st_size;
}
