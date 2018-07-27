/* 
   采用 epoll+线程池 实现，含有GET、POST， 可以发送html、picture、MP3、js、css、ico
   服务器可以稳定运行
 */

#include "webServer.h"
#include "task.h"

int main( int argc, char **argv ) {
    if( argc != 2 ) {
        cout << "Usage : ./server + port\n";
        return -1;
    }

    int port = atoi( argv[1] );
    WebServer webServer( port );
    webServer.run();

    return 0;
}