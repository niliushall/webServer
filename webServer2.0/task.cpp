#include "task.h"

void Task::doit() {
    char buf[ buffer_size ] = {0};

    while( int r = recv( accp_fd, buf, 1024, 0 ) ) {
        if( !r ) {
            cout << "browser exit.\n";
            return;
        } else if( r < 0 ) {  // 如果接收出错则继续接收数据
            continue;
        }

        string method, uri, version;
        stringstream ss;
        ss << buf;
        ss >> method >> uri >> version;  // 解析HTTP请求第一行

        cout << "method = " << method << endl;  // 方法：GET、POST
        cout << "uri = " << uri << endl;  // URL，请求文件地址
        cout << "version = " << version << endl << endl;  // HTTP版本

        if( method == "GET" ) {  // 为GET
            deal_get( uri );
        } else if( method == "POST" ) {  // 为POST
            deal_post( uri, buf );
        } else {
            string header = construct_header( 501, "Not Implemented", "text/plain" );
            send( accp_fd, header.c_str(), header.size(), 0 );
        }
        break;  // 只要处理完就退出循环，避免浏览器一直处于pending状态
    }
    // close( accp_fd );  // 任务完成直接close，不能在析构函数close(如果不delete task的话，
                          // 不delete task不够条用析构函数)
}

// num为状态码，info为状态描述，type为文件response类型
const string Task::construct_header( const int num, 
                        const string & info, const string & type ) {
    string response = "HTTP/1.1 " + to_string(num) + " " + info + "\r\n";
    response += "Server: niliushall\r\nContent-Type: " + type + ";charset=utf-8\r\n\r\n";
    return response;
}

int Task::deal_get( const string & uri ) {
    string filename = uri.substr(1);

    if( uri == "/" || uri == "/index.html" ) {
        send_file( "index.html", "text/html" );
    } else if( uri.find( ".jpg" ) != string::npos || uri.find( ".png" ) != string::npos ) {
        send_file( filename, "image/jpg" );
    } else if( uri.find( ".html" ) != string::npos ) {
        send_file( filename, "text/html" );
    } else if( uri.find( ".ico" ) != string::npos ) {
        send_file( filename, "image/x-icon" );
    } else if( uri.find( ".js" ) != string::npos ) {
        send_file( filename, "yexy/javascript" );
    } else if( uri.find( ".css" ) != string::npos ) {
        send_file( filename, "text/css" );
    } else if( uri.find( ".mp3" ) != string::npos ) {
        send_file( filename, "audio/mp3" );
    } else {
        send_file( "404.html", "text/html", 404, "Not Found" );
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
        send_file( "404.html", "text/html", 404, "Not Found" );
    }
}

// type对应response的Content-Type，num为状态码，info为状态描述
int Task::send_file( const string & filename, const string & type, 
                                const int num, const string & info ) {
    string header = construct_header( num, info, type );

    // send第二个参数只能是c类型字符串，不能使用string
    send( accp_fd, header.c_str(), header.size(), 0 );

    struct stat filestat;
    int ret = stat( filename.c_str(), &filestat );
    if( ret < 0 || !S_ISREG( filestat.st_mode ) ) {  // 打开文件出错或没有该文件
        cout << "file not found : " << filename << endl;
        send_file( "404.html", "text/html", 404, "Not Found" );
        return -1;
    }

    if( !filename.empty() ) {
        int fd = open( filename.c_str(), O_RDONLY );
        sendfile( accp_fd, fd, NULL, get_size( filename ) );
        close( fd );
    }
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