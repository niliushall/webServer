/* 采用 epoll+多线程 实现，可使用GET，可传输html、jpg、png、mp3 */

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
#include <pthread.h>
#include <sys/sendfile.h>
using namespace std;

const int max_event_num = 20;

class Server {
private:
    int port;
    int sock_fd;
    int epoll_fd;
    struct sockaddr_in server_addr;
public:
    Server( int p ) : sock_fd(0), port(p) { memset( &server_addr, 0, sizeof( server_addr ) ); }
    ~Server() { close( sock_fd ); }
    int accept_connection();
    static void * decode_request( void * accp_fd );
    static int send_file( int accp_fd, const string & filename, const string & type );
    static int send_400( int accp_fd );
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
    addfd( epoll_fd, false, sock_fd );

    while( true ) {
        ret = epoll_wait( epoll_fd, events, max_event_num, -1 );
        if( ret < 0 ) {
            cout << "epoll_wait error, line: " << __LINE__ << endl;
            return -1;
        }
        for( int i = 0; i < ret; i++ ) {
            int fd = events[i].data.fd;
            if( fd == sock_fd ) {  //新连接
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof( client_addr );
                int conn_fd = accept( fd, (struct sockaddr *)&client_addr, &client_addr_size );
                if( conn_fd < 0 ) {
                    cout << "accept error, line: " << __LINE__ << endl;
                    return -1;
                }
                addfd(epoll_fd, true, conn_fd);
// cout << "add\n";
            } else if( events[i].events & EPOLLIN ) {
// cout << "\nEPOLLIN\n";
                pthread_t thread;
                pthread_create( &thread, NULL, decode_request, (void *)&fd );
            } else {
                cout << "\nother\n";
            }
        }
    }

    close( sock_fd );
// cout << "exit\n";
    return 0;
}

void * Server::decode_request( void * conn_fd ) {
    char buf[1024] = {0};
    int accp_fd = *(int *)conn_fd;

    int r = recv( accp_fd, buf, 1024, 0 );
// cout << "r = " << r << endl;
    if( !r ) {
        cout << "browser exit.\n";
        close( accp_fd );
        // removefd( epoll_fd, accp_fd );
        return 0;
    }
    // cout << "request = \n" << buf << endl;

    string method, uri, version;
    stringstream ss;
    ss.clear();
    ss << buf;
    ss >> method >> uri >> version;

    cout << "method = " << method << endl;
    cout << "uri = " << uri << endl;
    cout << "version = " << version << endl << endl;

    // sleep(1);

    if( method == "GET" ) {  //为GET
        if( uri == "/" || uri == "/index.html" ) {
            send_file( accp_fd, "index.html", "text/html" );
        } else if( uri.find( ".jpg" ) != string::npos || uri.find( ".png" ) != string::npos ) {
            send_file( accp_fd, uri.substr(1), "image/jpg" );
        } else if( uri.find( ".html" ) != string::npos ) {
            send_file( accp_fd, uri.substr(1), "text/html" );
        } else if( uri.find( ".ico" ) != string::npos ) {
            send_file( accp_fd, uri.substr(1), "image/x-icon" );
        } else if( uri.find( ".js" ) != string::npos ) {
            send_file( accp_fd, uri.substr(1), "yexy/javascript" );
        } else if( uri.find( ".css" ) != string::npos ) {
            send_file( accp_fd, uri.substr(1), "text/css" );
        } else if( uri.find( ".mp3" ) != string::npos ) {
            /* int filefd = open( uri.substr(1).c_str(), O_RDONLY );
            struct stat stat_buf;
            fstat( filefd, &stat_buf );
            stringstream ss;
            ss << stat_buf.st_size;
            string num;
            ss >> num;
            string status( "HTTP/1.1 200 OK\r\n" );
            string header = "Server: niliushall\r\nContent-Type: audio/mp3\r\nContent-Length: "
                             + num + "\r\n";
            status += header;
            send( accp_fd, status.c_str(), status.size(), 0 );
            sendfile( accp_fd, filefd, NULL, stat_buf.st_size );
            close( filefd ); */
            send_file( accp_fd, uri.substr(1), "audio/mp3" );
        } else {
            string status( "HTTP/1.1 404 Not Found\r\n" );
            string header( "Server: niliushall\r\nContent-Type: text/plain;charset=utf-8\r\n\r\n" );
            status += header;
            send( accp_fd, status.c_str(), status.size(), 0 );
        }
    } else if( method == "POST" ) {
        
    } else {
        send_400( accp_fd );
    }
    close( accp_fd );
}

int Server::send_file( int accp_fd, const string & filename, const string & type ) {
    //? header为什么不能是\r\n\r\n
    string status( "HTTP/1.1 200 OK\r\n" );
    string header = "Server: niliushall\r\nContent-Type: " + type + ";charset=utf-8\r\n";
    string body, t;

    ifstream in( filename );
    while( getline( in, t ) ) {
        body += '\n';
        body += t;
    }
    in.close();
// cout << "\nsend:\n" << body << endl << endl;
    // send第二个参数只能是c类型字符串，不能使用string
    status += header + body;
    send( accp_fd, status.c_str(), status.size(), 0 );
cout << filename << " send finish to " << accp_fd << endl;
close( accp_fd );
    return 0;
}

int Server::send_400( int accp_fd ) {
    string response = "HTTP/1.1 200 METHOD ERROR\r\nServer: niliushall\r\nContent-Type: text/plain";
    send( accp_fd, response.c_str(), response.size(), 0 );
    close( accp_fd );
}

int main( int argc, char **argv ) {
    if( argc != 2 ) {
        cout << "Usage : ./a.out + port\n";
        return -1;
    }

    int port = atoi( argv[1] );
    Server test( port );
    test.accept_connection();
}