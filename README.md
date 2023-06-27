cpp framework for both rpc and http
=======================

### 环境依赖
c++17
taskflow 图引擎的基础框架
boost 用于conf文件解析

### 启动
git clone该代码库后，直接进入项目文件执行build.sh文件，会执行demo，在bin文件下生成可执行文件。

可以按照src下的*_main.cpp文件创建自己的rpc/http 服务端/客户端（目前还没写http客户端部分的代码，暂不需要，直接用postman或者浏览器向服务端发送请求就好了）。
在net_data.conf设置自己的网络参数
需要修改下CMake，需包含自己的源main文件。

build.sh会自动创建相关目录并执行编译

### 文件说明
.
├── CMakeLists.txt  
├── README.md
├── build.sh
├── conf  配置文件目录
│   ├── log.conf  日志配置
│   ├── net_data.conf  网络参数配置
├── http_resource   http服务器的资源
├── include  头文件目录
│   ├── common  公共工具
│   │   ├── block_queue.h  阻塞队列
│   │   ├── client.h  客户端
│   │   ├── conf.h  解析配置文件
│   │   ├── epoll.h  epoll相关的func
│   │   ├── log.h  日志
│   │   ├── net_data.h  网络参数
│   │   ├── serializer.h  rpc需要用到的序列化
│   │   ├── server.h  服务端
│   │   ├── socket.h  封装socket操作
│   │   ├── task.h    创建任务，由main调用
│   │   └── thread_pool.h 线程池
│   ├── service
│   │   ├── http_client.h http客户端（未实现）
│   │   ├── http_server.h http服务端
│   │   ├── rpc_client.h rpc客户端
│   │   ├── rpc_server.h rpc服务端
│   │   └── rpc_utils.h rpc工具
│   └── taskflow 并行任务执行框架
├── log 日志目录
└── src 源文件目录
    ├── common
    │   ├── block_queue.cpp
    │   ├── client.cpp
    │   ├── conf.cpp
    │   ├── epoll.cpp
    │   ├── log.cpp
    │   ├── server.cpp
    │   ├── socket.cpp
    │   └── thread_pool.cpp
    ├── http_server_main.cpp  // 创建httpserver的demo
    ├── rpc_client_main.cpp   // 创建rpcclient的demo
    ├── rpc_server_main.cpp   // 创建rpcserver的demo
    └── service
        ├── http_server.cpp
        └── rpc_server.cpp




