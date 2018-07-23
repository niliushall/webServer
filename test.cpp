/* 采用epoll实现，可传输html、jpg */

#include <iostream>
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
using namespace std;

const int port = 8000;
const int max_event_num = 5;

class Server {
private:
    int sock_fd;
    // int accp_fd;
    int epoll_fd;
    struct sockaddr_in server_addr;
public:
    Server() : sock_fd(0) { memset( &server_addr, 0, sizeof( server_addr ) ); }
    ~Server() { close( sock_fd ); }
    int accept_connection();
    int decode_request( int accp_fd );
    int send_html( int accp_fd );
    void addfd( bool oneshot );
    int setnonblocking();
};

int Server::accept_connection() {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons( port );
    server_addr.sin_addr.s_addr = htonl( INADDR_ANY );

    sock_fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( sock_fd < 0 ) {
        cout << "socket error, line " << __LINE__ << endl;
        exit(-1);
    }

    int ret = bind( sock_fd, (struct sockaddr *)&server_addr, sizeof( server_addr ) );
    if( ret < 0 ) {
        cout << "bind error, line " << __LINE__ << endl;
        exit(-1);
    }

    ret = listen( sock_fd, 5 );
    if( ret < 0 ) {
        cout << "listen error, line " << __LINE__ << endl;
        exit(-1);
    }

    epoll_event events[max_event_num];
    epoll_fd = epoll_create(1024);
    if( epoll_fd < 0 ) {
        cout << "epoll_create error, line: " << __LINE__ << endl;
        exit(-1);
    }
    addfd( false );

    while( true ) {
        ret = epoll_wait( epoll_fd, events, max_event_num, -1 );
        if( ret < 0 ) {
            cout << "epoll_wait error, line: " << __LINE__ << endl;
            exit(-1);
        }

        for( int i = 0; i < ret; i++ ) {
            int fd = events[i].data.fd;
            if( fd == sock_fd ) {  //新连接
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof( client_addr );
                int conn_fd = accept( fd, (struct sockaddr *)&client_addr, &client_addr_size );
                if( conn_fd < 0 ) {
                    cout << "accept error, line: " << __LINE__ << endl;
                    exit(-1);
                }
                addfd(true);
                decode_request( conn_fd );
                close( conn_fd );
            } else if( events[i].events & EPOLLIN ) {
                //
            }
        }
    }

    /* struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof( client_addr );
    accp_fd = accept( sock_fd, (struct sockaddr *)&client_addr, &client_addr_size ); */

    return 0;
}

int Server::decode_request( int accp_fd ) {
    char buf[1024] = {0};
    
    int r = recv( accp_fd, buf, 1024, 0 );
    // cout << "r = " << r << endl;
    if( !r ) {
        cout << "browser exit.\n";
        close( accp_fd );
        exit(-1);
    }
    cout << "buf = \n" << buf << endl;

    char method[1024] = {0}, uri[1024] = {0}, version[1024] = {0};
    sscanf( buf, "%s %s %s", method, uri, version );
    /* string method, uri, version;
    stringstream ss;
    ss << uri;
    ss >> method >> uri >> version;
    cout << "method = " << method << endl;
    cout << "uri = " << uri << endl;
    cout << "version = " << version << endl; */

    // sleep(1);

    if( !strcmp( method, "GET" ) ) {  //为GET
        if( !strcmp( "/", uri ) || !strcmp( "/homepage.html", uri ) ) {
            send_html( accp_fd );
        } else if( strstr( uri, ".jpg" ) ) {
            string status( "HTTP/1.1 200 OK\r\n" );
            string header( "Server: niliushall\r\nContent-Type: image/jpg;charset=utf-8\r\n" );
            string body, t;
            
            string filename( uri );
            ifstream fin( filename.substr(1) );
            if( !fin.is_open() ) {
                cout << "file " << uri << " can't open.\n";
                exit(-1);
            }
            
            //文件以getline读取，不能直接fin >> t, 否则只读空格之前的
            while( getline( fin, t ) ) {
                body += '\n';
                body += t;
            }

            status += header + body;
            send( accp_fd, status.c_str(), status.size(), 0 );
            fin.close();
            // close( accp_fd );
        }
    }
}

int Server::send_html( int accp_fd ) {
    string status( "HTTP/1.1 200 OK\r\n" );
    string header( "Server: niliushall\r\nContent-Type: text/html;charset=utf-8\r\n\r\n" );
    string body, t;

    ifstream in( "homepage.html" );
    while( getline( in, t ) ) {
        body += '\n';
        body += t;
    }
    in.close();
    
    /* send第二个参数只能是c类型字符串，不能使用string */
    send( accp_fd, status.c_str(), status.size(), 0 );
    send( accp_fd, header.c_str(), header.size(), 0 );
    send( accp_fd, body.c_str(), body.size(), 0 );

    return 0;
}

void Server::addfd( bool oneshot ) {
    epoll_event event;
    event.data.fd = sock_fd;
    event.events = EPOLLIN | EPOLLET;
    if( oneshot ) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl( epoll_fd, EPOLL_CTL_ADD, sock_fd, &event );
    setnonblocking( );
}

int Server::setnonblocking() {
    int old_option = fcntl( sock_fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( sock_fd, F_SETFL, new_option );
    return old_option;
}

int main() {
    Server test;
    test.accept_connection();
    // test.decode_request();
}