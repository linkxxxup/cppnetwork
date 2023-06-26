#include "conf.h"

wut::zgy::cppnetwork::Conf::Conf(const char* ini_file){
    if (access(ini_file, 0) == 0) {
        this->_err_code = 0;
        boost::property_tree::ini_parser::read_ini(ini_file, this->_pt);
    } else {
        this->_err_code = 1;
    }
}

short wut::zgy::cppnetwork::Conf::get_err_code(){
    return this->_err_code;
}

std::string wut::zgy::cppnetwork::Conf::get(const char* path){
    if (this->_err_code == 0) {
        return this->_pt.get<std::string>(path);
    } else {
        return "";
    }
}

boost::property_tree::ptree wut::zgy::cppnetwork::Conf::get_pt() {
    return this->_pt;
}
