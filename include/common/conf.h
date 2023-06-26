#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <string>

namespace wut::zgy::cppnetwork{

class Conf{
public:
    explicit Conf(const char*);
    Conf() = default;
    std::string get(const char* path); // 直接解析
    short int get_err_code();
    boost::property_tree::ptree get_pt();

private:
    short int _err_code;
    boost::property_tree::ptree _pt;
};

}
