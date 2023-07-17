#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <map>

#include "server.h"
#include "serializer.h"
#include "rpc_utils.h"

namespace wut::zgy::cppnetwork{
class HttpServer : public Server{
    // 状态码简写
    typedef NetData::HTTP_ERR_CODE HTTP_ERR_CODE;
    typedef NetData::LINE_STATUS LINE_STATUS;
    typedef NetData::CHECK_STATE CHECK_STATE;
    typedef NetData::METHOD METHOD;
    // char数组大小设定
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    // 状态信息
    const char *ok_200_title = "OK";
    const char *error_400_title = "Bad Request";
    const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
    const char *error_403_title = "Forbidden";
    const char *error_403_form = "You do not have permission to get file form this server.\n";
    const char *error_404_title = "Not Found";
    const char *error_404_form = "The requested file was not found on this server.\n";
    const char *error_500_title = "Internal Error";
    const char *error_500_form = "There was an unusual problem serving the request file.\n";
    // http资源所在目录
    const char* _root_path = "../http_resource";
public:
    // TODO: 应该不能用单例模式，要不同一时间只能处理一个实例
    static HttpServer *get_instance()
    {
        static HttpServer instance;
        return &instance;
    }

    int receive(int sockfd) override;
    int deal_read(int sockfd, int len) override;
    int deal_write(int sockfd) override;
    void connect_registry();

    // 数据进行初始化
    void init_data(int sockfd);
    // 解析http函数
    // 读过程
    HTTP_ERR_CODE process_read(int sockfd);
    // 返回http请求每一行的字符串
    char *get_line(int sockfd);
    // 从状态机, 解析一行数据
    LINE_STATUS parse_line(int sockfd);
    // 主状态机
    // 解析请求行
    HTTP_ERR_CODE parse_request_line(char *text, int sockfd);
    // 解析请求头
    HTTP_ERR_CODE parse_headers(char *text, int sockfd);
    // 解析请求内容
    HTTP_ERR_CODE parse_content(char *text, int sockfd);
    // 处理请求
    HTTP_ERR_CODE deal_request(int sockfd);

    // 写http函数
    bool process_write(HTTP_ERR_CODE, int sockfd);
    bool add_response(int sockfd, const char *format, ...);
    bool add_content(const char *content, int sockfd);
    bool add_status_line(int status, const char *title, int sockfd);
    bool add_headers(int content_length, int sockfd);
    bool add_content_type(int sockfd);
    bool add_content_length(int content_length, int sockfd);
    bool add_linger(int sockfd);
    bool add_blank_line(int sockfd);

    // 为每个任务创建自己的数据结构存储信息，为了共用httpserver的处理函数，
    // 如果不引入该类内类型，由于httpserverr为单例模式，只能同时存在一个任务的信息
    class EveryTask{
    public:
        EveryTask(int sockfd): _sockfd(sockfd){
            _check_state = CHECK_STATE::CHECK_STATE_REQUESTLINE;
            memset(_recv_buf, 0, sizeof (_recv_buf));
            memset(_send_buf, 0, sizeof (_send_buf));
            _start_line = 0;
            _checked_idx = 0;
            _read_idx = 0;
            _write_idx = 0;
            _bytes_to_send = 0;
            _bytes_have_send = 0;
            _method = METHOD ::GET;
            _url = nullptr;
            _version = nullptr;
            _content_length = 0;
            _linger = false;
            _host = nullptr;
            _content = nullptr;
        }
        ~EveryTask() = default;

        int _sockfd;
        CHECK_STATE _check_state; // 解析http请求行，请求头和请求内容的状态, 主状态机
        char _recv_buf[READ_BUFFER_SIZE];
        char _send_buf[WRITE_BUFFER_SIZE];
        long _start_line; // 是每一个数据行在_recv_buf中的起始位置
        long _checked_idx; // 表示从状态机在_read_buf中读取的位置
        long _read_idx;
        long _write_idx;
        size_t _bytes_to_send;
        size_t _bytes_have_send;
        METHOD _method; // HTTP请求方法
        // 解析http请求后存放变量
        char* _url;
        char* _version;
        int _content_length;
        bool _linger;
        char* _host;
        char* _content;
        // 发送的文件信息
        char _file_path[FILENAME_LEN];
        char *_mmap_file;
        struct stat _file_stat;
        struct iovec _iv[2];
        int _iv_count;
    };

private:
    HttpServer() = default;
    ~HttpServer()override = default;
    std::map<int, EveryTask> _task;
    std::mutex _mutex;
    Socket _register_fd;
};

}