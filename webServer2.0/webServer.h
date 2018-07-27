#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include "task.h"
#include "threadPool.h"
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

#endif