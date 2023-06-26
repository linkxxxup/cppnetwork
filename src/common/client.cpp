#include "client.h"


namespace wut::zgy::cppnetwork{
    int Client::init(NetData *net_data){
        _net_data = net_data;
        _ip = _net_data->_ip;
        _port = _net_data->_port;
        _cli_fd = Socket(_ip, _port);
        return 0;
    }

    int Client::run() {
        if((_cli_fd._socket_fd = ::socket(AF_INET, SOCK_STREAM, 0)) == -1){
            LOG_ERROR("%s", strerror(errno));
            return 1;
        }
        _cli_fd.set_recv_buf(10 * 1024);
        _cli_fd.set_send_buf(10 * 1024);
//        _cli_fd.set_non_block();
        _cli_fd.connect();
//        while(1){
//            std::string str;
//            std::cin >> str;
//            if(str == "#") break;
//            _cli_fd.send(str.c_str(), strlen(str.c_str()));
//        }
        return 0;
    }

    int Client::close() {
        _cli_fd.close();
        return 0;
    }


}