#pragma once

#include <string>
#include <unordered_map>
#include "conf.h"

// TODO: 设置一个能解析dag_conf的类
// 目前不用到该代码，只能手工从func中的conf文件中生成一个图。
namespace wut::zgy::cppnetwork{
class Dag{
public:
    explicit Dag(Conf&& dag_conf): _dag_conf(dag_conf){}
    ~Dag() = default;
    int dag_init() noexcept{
        auto ptree = _dag_conf.get_pt();
        _ptree = ptree.get_child("");
        // 解析两层
        for(auto it = _ptree.begin(); it != _ptree.end(); ++it){
            _sub_ptree = it->second;
            for(auto sub_it = _sub_ptree.begin(); sub_it != _sub_ptree.end(); ++sub_it){
                str_sub_args[sub_it->first] = sub_it->second.get<std::string>("");
            }
            str_args[it->first] = str_sub_args;
        }

        std::unordered_map<std::string, std::string> map_args;


        return 0;
    }

private:

    Conf _dag_conf;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> str_args;
    std::unordered_map<std::string, std::string> str_sub_args;
    boost::property_tree::ptree _ptree;
    boost::property_tree::ptree _sub_ptree;
};
}
