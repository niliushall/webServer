#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include "task.h"
using namespace std;

const int max_event_num = 20;

class WebServer {
private:
    int port;
    int sock_fd;
    int epoll_fd;
    struct sockaddr_in server_addr;
public:
    WebServer( int p ) : sock_fd(0), port(p) { memset( &server_addr, 0, sizeof( server_addr ) ); }
    ~WebServer() { close( sock_fd ); }
    int run();
};

int setnonblocking( int fd ) {
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}


void addfd( int epoll_fd, bool oneshot, int fd ) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if( oneshot ) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &event );
    setnonblocking( fd );
}

void removefd( int epollfd, int fd ) {
    epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
    close( fd );
}

int WebServer::run() {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons( port );
    server_addr.sin_addr.s_addr = htonl( INADDR_ANY );

    sock_fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( sock_fd < 0 ) {
        cout << "socket error, line " << __LINE__ << endl;
        return -1;
    }

    int ret = bind( sock_fd, (struct sockaddr *)&server_addr, sizeof( server_addr ) );
    if( ret < 0 ) {
        cout << "bind error, line " << __LINE__ << endl;
        return -1;        
    }

    ret = listen( sock_fd, 5 );
    if( ret < 0 ) {
        cout << "listen error, line " << __LINE__ << endl;
        return -1;        
    }

    ThreadPool<Task> threadpool(20);

    /*ThreadPool< Task > *threadpool;
    try {
        threadpool = new ThreadPool<Task>;
    } catch(...) {
        cout << "init threadpool error\n";
        return -1;
    }*/
    

    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof( client_addr );
        int conn_fd = accept( sock_fd, (struct sockaddr *)&client_addr, &client_addr_size );
        if( conn_fd < 0 ) {
            cout << "accept error, line: " << __LINE__ << endl;
            exit(-1);
        }
cout << "conn_fd = " << conn_fd << endl;
        Task *task = new Task(conn_fd);
        threadpool.append( task );
    }
    return 0;
}

#endif