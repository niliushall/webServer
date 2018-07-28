### 项目概述

该项目是一个简单的webServer，使用 `epoll + threadpool` 实现，支持GET、POST方法，可以实现html、jpg、png、ico、MP3、js、css等文件的解析，在POST方法的处理中使用CGI服务器进行计算并返回结果。

### 使用说明

该项目使用时，需要在目录中放置一个mp3文件，并命名为1.mp3，具体位置为webServer2.0目录下。使用make进行编译连接源文件，使用：`./server port`运行程序。其中，`port`为端口号。

以8080端口为例，终端输入：

```
./server 8080
```

打开浏览器，输入：

```
localhost:8080
```

即可看到网页显示。

### 版本说明

- `test.cpp`为最初版本，使用 `epoll + 多线程` 实现，可以完成基础功能，支持GET方法，可传输html、jpg、png、mp3
- `webServer1.0`使用 `threadpool` 实现，支持GET、POST方法，可以解析html、jpg、png、ico、MP3、js、css等，并加入了CGI服务器，可以稳定运行
- `webServer2.0`使用 `epoll + threadpool`实现，支持GET、POST方法，可以解析html、jpg、png、ico、MP3、js、css等，使用CGI服务器进行数据的计算并返回网页给浏览器，可以稳定运行
