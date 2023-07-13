#include "http_server.h"

namespace wut::zgy::cppnetwork{
    // 和rpcserver接收函数一致
    // 调用时使用_read_idx = receive(sockfd)
    int HttpServer::receive(int sockfd) {
        _mutex.lock();
        _task.insert({sockfd, EveryTask(sockfd)});
        _mutex.unlock();
        
        EveryTask &task = _task.at(sockfd);
        char *buf = task._recv_buf;
        memset(task._recv_buf, 0, sizeof(task._recv_buf));
        int len;
        int pos = 0;
        while(true){
            len = _listen_fd.receive(sockfd, buf+pos, sizeof(task._recv_buf)-pos);
            task._read_idx = pos; //http需要接收请求的长度
            if(len < 0){
                if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                    LOG_INFO("EAGAIN is trigger")
                    return pos;
                }
                return pos;
            }else if(len == 0){
                return pos;
            }else{
                pos += len;
            }
        }
    }

    int HttpServer::deal_read(int sockfd, int len) {
        HTTP_ERR_CODE ret_read = process_read(sockfd);
        if(ret_read == HTTP_ERR_CODE::NO_REQUEST){
            if(_is_et_conn){
                _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLET|EPOLLERR|EPOLLRDHUP);
            }else{
                _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLERR|EPOLLRDHUP);
            }
//            LOG_INFO("请求不完整，继续读")
            return 0;
        }
        bool ret_write = process_write(ret_read, sockfd);
        if(!ret_write){
            LOG_ERROR("生成失败http响应报文")
            if(_is_et_conn){
                _epoll_fd->mod_fd(sockfd, EPOLLOUT|EPOLLONESHOT|EPOLLET|EPOLLERR|EPOLLRDHUP);
            }else{
                _epoll_fd->mod_fd(sockfd, EPOLLOUT|EPOLLONESHOT|EPOLLERR|EPOLLRDHUP);
            }
//            close(sockfd);
            return 0;
        }
        if(_is_et_conn){
            _epoll_fd->mod_fd(sockfd, EPOLLOUT|EPOLLONESHOT|EPOLLET|EPOLLERR|EPOLLRDHUP);
        }else{
            _epoll_fd->mod_fd(sockfd, EPOLLOUT|EPOLLONESHOT|EPOLLERR|EPOLLRDHUP);
        }
        return 0;
    }

    int HttpServer::deal_write(int sockfd) {
        EveryTask &task = _task.at(sockfd);
        size_t temp ;
        if (task._bytes_to_send == 0){
            if(_is_et_conn){
                _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLET|EPOLLERR|EPOLLRDHUP);
            }else{
                _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLERR|EPOLLRDHUP);
            }
            init_data(sockfd);
            return 0;
        }
        while (true){
            temp = writev(sockfd, task._iv, task._iv_count);
            if (temp < 0){
                if (errno == EAGAIN || errno == EWOULDBLOCK){
                    if(_is_et_conn){
                        _epoll_fd->mod_fd(sockfd, EPOLLOUT|EPOLLONESHOT|EPOLLET|EPOLLERR|EPOLLRDHUP);
                    }else{
                        _epoll_fd->mod_fd(sockfd, EPOLLOUT|EPOLLONESHOT|EPOLLERR|EPOLLRDHUP);
                    }
                    return 0;
                }
                munmap(task._mmap_file, task._file_stat.st_size);
                task._mmap_file = nullptr;
                return 1;
            }
            task._bytes_have_send += temp;
            task._bytes_to_send -= temp;
            if (task._bytes_have_send >= task._iv[0].iov_len){
                task._iv[0].iov_len = 0;
                task._iv[1].iov_base = task._mmap_file + (task._bytes_have_send - task._write_idx);
                task._iv[1].iov_len = task._bytes_to_send;
            }
            else{
                task._iv[0].iov_base = task._send_buf + task._bytes_have_send;
                task._iv[0].iov_len = task._iv[0].iov_len - task._bytes_have_send;
            }
            // 响应全部发完
            if (task._bytes_to_send <= 0){
                munmap(task._mmap_file, task._file_stat.st_size);
                task._mmap_file = nullptr;
                if(_is_et_conn){
                    _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLET|EPOLLERR|EPOLLRDHUP);
                }else{
                    _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLERR|EPOLLRDHUP);
                }

                if (task._linger){
                    init_data(sockfd);
                    return 0;
                }
                else{
                    return 1;
                }
            }
        }
    }

    void HttpServer::init_data(int sockfd) {
        EveryTask &task = _task.at(sockfd);
        task._check_state = CHECK_STATE::CHECK_STATE_REQUESTLINE;
        memset(task._recv_buf, 0, sizeof (task._recv_buf));
        memset(task._send_buf, 0, sizeof (task._send_buf));
        task._start_line = 0;
        task._checked_idx = 0;
        task._read_idx = 0;
        task._write_idx = 0;
        task._bytes_to_send = 0;
        task._bytes_have_send = 0;
        task._method = METHOD ::GET;
        task._url = nullptr;
        task._version = nullptr;
        task._content_length = 0;
        task._linger = false;
        task._host = nullptr;
        task._content = nullptr;
    }

    NetData::HTTP_ERR_CODE HttpServer::process_read(int sockfd) {
        EveryTask &task = _task.at(sockfd);
        char *text;
        // 设置初始状态，包括行状态和整体解析请求体状态
        LINE_STATUS line_status = LINE_STATUS::LINE_OK;
        HTTP_ERR_CODE status;
        while(task._check_state == CHECK_STATE::CHECK_STATE_CONTENT && line_status == LINE_STATUS::LINE_OK
              || ((line_status = parse_line(sockfd)) == LINE_STATUS::LINE_OK)){
            text = get_line(sockfd);
            task._start_line = task._checked_idx;
            switch(task._check_state){
                case CHECK_STATE::CHECK_STATE_REQUESTLINE:{
                    status = parse_request_line(text, sockfd);
                    if(status == HTTP_ERR_CODE::BAD_REQUEST){
                        LOG_INFO("CHECK_STATE_REQUESTLINE: BAD_REQUEST")
                        return HTTP_ERR_CODE::BAD_REQUEST;
                    }
                    break;
                }
                case CHECK_STATE::CHECK_STATE_HEADER:{
                    status = parse_headers(text, sockfd);
                    if(status == HTTP_ERR_CODE::BAD_REQUEST){
                        LOG_INFO("CHECK_STATE_HEADER: BAD_REQUEST")
                        return HTTP_ERR_CODE :: BAD_REQUEST;
                    }else if (status == HTTP_ERR_CODE::GET_REQUEST){
                        return deal_request(sockfd);
                    }
                    break;
                }
                case CHECK_STATE::CHECK_STATE_CONTENT:{
                    status = parse_content(text, sockfd);
                    if(status == HTTP_ERR_CODE::GET_REQUEST){
                        return deal_request(sockfd);
                    }
                    //解析完消息体即完成报文解析，避免再次进入循环
                    line_status= LINE_STATUS::LINE_OPEN;
                    break;
                }
                default:{
                    LOG_INFO("INTERNAL_ERROR")
                    return HTTP_ERR_CODE ::INTERNAL_ERROR;
                }
            }
        }
//        LOG_INFO("NO_REQUEST")
        return HTTP_ERR_CODE::NO_REQUEST;
    }

    char *HttpServer::get_line(int sockfd) {
        EveryTask &task = _task.at(sockfd);
        return task._recv_buf + task._start_line;
    }

    NetData::LINE_STATUS HttpServer::parse_line(int sockfd) {
        EveryTask &task = _task.at(sockfd);
        char cur;
        for(; task._checked_idx < task._read_idx; ++task._checked_idx){
            cur = task._recv_buf[task._checked_idx];
            if(cur == '\r'){
                //下一个字符达到了buffer结尾，则接收不完整，需要继续接收
                if((task._checked_idx + 1) == task._read_idx){
                    return LINE_STATUS ::LINE_OPEN;
                }
                //下一个字符是\n，将\r\n改为\0\0
                else if(task._recv_buf[task._checked_idx + 1] == '\n'){
                    task._recv_buf[task._checked_idx++] = '\0';
                    task._recv_buf[task._checked_idx++] = '\0';
                    return LINE_STATUS ::LINE_OK;
                }
                return LINE_STATUS ::LINE_BAD;
            }
            else if(cur == '\n'){
                if(task._checked_idx > 1 && task._recv_buf[task._checked_idx-1] == '\r'){
                    task._recv_buf[task._checked_idx - 1] = '\0';
                    task._recv_buf[task._checked_idx++] = '\0';
                    return LINE_STATUS ::LINE_OK;
                }
                return LINE_STATUS ::LINE_BAD;
            }
        }
        return LINE_STATUS ::LINE_OPEN;
    }

    NetData::HTTP_ERR_CODE HttpServer::parse_request_line(char *text, int sockfd) {
        EveryTask &task = _task.at(sockfd);
        // 请求行格式:GET / HTTP/1.1
        //请求行中最先含有空格和\t任一字符的位置并返回
        task._url = strpbrk(text, " \t");
        if(!task._url){
            return HTTP_ERR_CODE ::BAD_REQUEST;
        }
        //将该位置改为\0，用于将前面数据取出
        *task._url++ = '\0';
        //取出数据，并通过与GET和POST比较，以确定请求方式
        char *method = text;
        if(strcasecmp(method, "POST") == 0){
            task._method = METHOD ::POST;
        }else if(strcasecmp(method, "GET") == 0){
            task._method = METHOD ::GET;
        }else{
            return HTTP_ERR_CODE ::BAD_REQUEST;
        }
        // task._url向后偏移，通过查找，继续跳过空格和\t字符，指向请求资源的第一个字符
        task._url += strspn(task._url, " \t");
        //使用与判断请求方式的相同逻辑，判断HTTP版本号
        task._version = strpbrk(task._url, " \t");
        if(!task._version){
            return HTTP_ERR_CODE ::BAD_REQUEST;
        }
        *task._version++ = '\0';
        task._version += strspn(task._version, " \t");
        // 只支持http1.1
        if(strcasecmp(task._version, "HTTP/1.1") != 0){
            return HTTP_ERR_CODE ::BAD_REQUEST;
        }
        //对请求资源前7个字符进行判断
        //这里主要是有些报文的请求资源中会带有http://，这里需要对这种情况进行单独处理
        if(strncasecmp(task._url,"http://",7)==0)
        {
            task._url+=7;
            task._url=strchr(task._url,'/');
        }
        //同样增加https情况
        if(strncasecmp(task._url,"https://",8)==0)
        {
            task._url+=8;
            task._url=strchr(task._url,'/');
        }
        if(!task._url || task._url[0] != '/'){
            return HTTP_ERR_CODE ::BAD_REQUEST;
        }
        // 转移状态
        task._check_state = CHECK_STATE ::CHECK_STATE_HEADER;
        return HTTP_ERR_CODE ::NO_REQUEST;

    }

    NetData::HTTP_ERR_CODE HttpServer::parse_headers(char *text, int sockfd) {
        EveryTask &task = _task.at(sockfd);
        if(text[0] == '\0'){
            // 如果请求内容长度不为0，则需要继续解析请求内容
            if(task._content_length != 0){
                task._check_state = CHECK_STATE ::CHECK_STATE_CONTENT;
                return HTTP_ERR_CODE ::NO_REQUEST;
            }
            return HTTP_ERR_CODE ::GET_REQUEST;
        }
        else if(strncasecmp(text, "Connection:", 11) == 0){
            text += 11;
            //跳过空格和\t字符
            text += strspn(text, " \t");
            if(strcasecmp(text, "keep-alive") == 0){
                task._linger = true;
            }
        }
        else if(strncasecmp(text, "Content-length:", 15) == 0){
            text += 15;
            text += strspn(text, "\t");
            task._content_length= atoi(text);
        }
        else if(strncasecmp(text, "Host:", 5) == 0){
            text += 5;
            text += strspn(text, " \t");
            task._host = text;
        }
        else {
            LOG_INFO("oop!unknow header: %s",text);
        }
        return HTTP_ERR_CODE ::NO_REQUEST;
    }

    NetData::HTTP_ERR_CODE HttpServer::parse_content(char *text, int sockfd) {
        EveryTask &task = _task.at(sockfd);
        //判断buffer中是否读取了消息体
        if(task._read_idx >= task._content_length + task._checked_idx){
            text[task._content_length] = '\0';
            task._content = text;
            return HTTP_ERR_CODE ::GET_REQUEST;
        }
        return HTTP_ERR_CODE ::NO_REQUEST;
    }

    NetData::HTTP_ERR_CODE HttpServer::deal_request(int sockfd) {
        EveryTask &task = _task.at(sockfd);
        switch (task._method) {
            case METHOD::POST:{
                //TODO: 创建一个文件来存放POST的内容,
                return HTTP_ERR_CODE ::GET_REQUEST;
            }
            case METHOD::PUT:{
                //TODO: 创建一个文件来存放PUT的内容,
                return HTTP_ERR_CODE ::GET_REQUEST;
            }
            case METHOD::GET:{
                strcpy(task._file_path, _root_path);
                size_t len = strlen(_root_path);
                //找到task._url中/的位置
                const char *p = strrchr(task._url, '/');
                strcat(task._file_path, p);
                // 如果url为/ ,则拼接index.html，以显示默认内容
                if(task._file_path[len + 1] == '\0'){
                    strcat(task._file_path, "index.html");
                }else{
                    strcat(task._file_path, ".html");
                }
                if (stat(task._file_path, &task._file_stat) < 0)
                    return HTTP_ERR_CODE ::NO_RESOURCE;
                if (!(task._file_stat.st_mode & S_IROTH))
                    return HTTP_ERR_CODE ::FORBIDDEN_REQUEST;
                if (S_ISDIR(task._file_stat.st_mode))
                    return HTTP_ERR_CODE ::BAD_REQUEST;
                int fd = open(task._file_path, O_RDONLY);
                task._mmap_file = (char *)mmap(nullptr, task._file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
                close(fd);
                return HTTP_ERR_CODE ::FILE_REQUEST;
            }
        }
    }

    bool HttpServer::process_write(NetData::HTTP_ERR_CODE code, int sockfd) {
        EveryTask &task = _task.at(sockfd);
        switch(code){
            case HTTP_ERR_CODE::INTERNAL_ERROR:{
                add_status_line(500, error_500_title, sockfd);
                add_headers(strlen(error_500_form), sockfd);
                if(!add_content((error_500_form), sockfd)){
                    return false;
                }
                break;
            }
            case HTTP_ERR_CODE::BAD_REQUEST:{
                add_status_line(404, error_404_title, sockfd);
                add_headers(strlen(error_404_form), sockfd);
                if (!add_content(error_404_form, sockfd))
                    return false;
                break;
            }
            case HTTP_ERR_CODE::FORBIDDEN_REQUEST:{
                add_status_line(403, error_403_title, sockfd);
                add_headers(strlen(error_403_form), sockfd);
                if (!add_content(error_403_form, sockfd)){
                    return false;
                }
                break;
            }
            case HTTP_ERR_CODE::FILE_REQUEST:{
                add_status_line(200, ok_200_title, sockfd);
                // 如果需要返回的响应内容是一个文件，则分两部分发送
                if(task._file_stat.st_size != 0){
                    add_headers(task._file_stat.st_size, sockfd);
                    task._iv[0].iov_base = task._send_buf;
                    task._iv[0].iov_len = task._write_idx;
                    task._iv[1].iov_base = task._mmap_file;
                    task._iv[1].iov_len = task._file_stat.st_size;
                    task._iv_count = 2;
                    task._bytes_to_send = task._write_idx + task._file_stat.st_size;
                    return true;
                }else{
                    const char *ok_string = "<html><body>你请求了该服务器，但是服务器没有你需要的访问的页面，所以你会看到这句话，证明我们是没问题的^0^</body></html>";
                    add_headers(strlen(ok_string), sockfd);
                    if (!add_content(ok_string, sockfd)){
                        return false;
                    }
                }
                break;
            }
            default: {
                return false;
            }
        }
        task._iv[0].iov_base = task._send_buf;
        task._iv[0].iov_len = task._write_idx;
        task._iv_count = 1;
        task._bytes_to_send = task._write_idx;
        return true;
    }

    bool HttpServer::add_response(int sockfd, const char *format, ...) {
        EveryTask &task = _task.at(sockfd);
        if (task._write_idx >= WRITE_BUFFER_SIZE){
            LOG_ERROR("发送空间有限，发送的字段超出发送的最大长度")
            return false;
        }
        va_list arg_list;
        va_start(arg_list, format);
        int len = vsnprintf(task._send_buf + task._write_idx, WRITE_BUFFER_SIZE - 1 - task._write_idx, format, arg_list);
        if (len >= (WRITE_BUFFER_SIZE - 1 - task._write_idx))
        {
            LOG_ERROR("发送空间有限，发送的字段超出发送的最大长度")
            va_end(arg_list);
            return false;
        }
        task._write_idx += len;
        va_end(arg_list);
        return true;
    }

    bool HttpServer::add_status_line(int status, const char *title, int sockfd) {
        return add_response(sockfd, "%s %d %s\r\n", "HTTP/1.1", status, title);
    }

    bool HttpServer::add_headers(int content_length, int sockfd) {
        return add_content_length(content_length, sockfd) && add_linger(sockfd) &&
               add_blank_line(sockfd);
    }

    bool HttpServer::add_content_length(int content_length, int sockfd) {
        return add_response(sockfd, "Content-Length:%d\r\n", content_length);
    }

    bool HttpServer::add_blank_line(int sockfd) {
        return add_response(sockfd, "%s", "\r\n");
    }

    bool HttpServer::add_linger(int sockfd) {
        EveryTask &task = _task.at(sockfd);
        return add_response(sockfd, "Connection:%s\r\n", task._linger ? "keep-alive" : "close");
    }

    bool HttpServer::add_content_type(int sockfd) {
        return add_response(sockfd, "Content-Type:%s\r\n", "text/html");
    }

    bool HttpServer::add_content(const char *content, int sockfd) {
        return add_response(sockfd, "%s", content);
    }






}