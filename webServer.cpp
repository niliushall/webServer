// #include "task.h"
#include "webServer.h"
#include "task.h"

int main( int argc, char **argv ) {
    if( argc != 2 ) {
        cout << "Usage : webServer + port\n";
        return -1;
    }

    int port = atoi( argv[1] );
    WebServer webServer( port );
    webServer.run();

    return 0;
}