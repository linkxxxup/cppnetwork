#pragma once

#include "serializer.h"

namespace wut::zgy::cppnetwork{
    template<typename T>
    struct type_xx{	typedef T type; };
    template<>
    struct type_xx<void>{ typedef int8_t type; };

    // 打包帮助模板, 为客户端调用
    template<typename Tuple, std::size_t... Is>
    // std::index_sequence<Is...>接收std::index_sequence_for<Args....>{}传来的0到N-1个index
    void package_params_impl(Serializer& ds, const Tuple& t, std::index_sequence<Is...>)
    {
        //用了逗号表达式，例如a=((b=c), d)会执行b=c，然后返回d，赋值给a
        // std::get<>()通过索引取出元组t中的值
        // << 为serializer中重写的运算符
        std::initializer_list<int>{((ds << std::get<Is>(t)), 0)...};
    }
    template<typename... Args>
    // std::tuple<Args...> 创建一个类型为<Args...>的元组
    void package_params(Serializer& ds, const std::tuple<Args...>& t)
    {
        // std::index_sequence_for<Args...> 生成0-size...(Args)个整数
        package_params_impl(ds, t, std::index_sequence_for<Args...>{});
    }

    // 用tuple做参数调用函数模板类, 为服务端调用
    template<typename Function, typename Tuple, std::size_t... Index>
    decltype(auto) invoke_impl(Function&& func, Tuple&& t, std::index_sequence<Index...>)
    {
        // std::forward<Tuple>(t) 完美转发参数t
        // std::get<Index>得到tuple中第index个元素，... 为解参数包
        return func(std::get<Index>(std::forward<Tuple>(t))...);
    }
    // 调用invoke的例子
    /*int add(int a, int b)
    {
        return a + b;
    }
    std::tuple<int, int> t = std::make_tuple(1, 2);
    std::cout << invoke(add, t) << std::endl;*/
    template<typename Function, typename Tuple>
    decltype(auto) invoke(Function&& func, Tuple&& t)
    {
        // decay用来移除类型中的一些特性，比如引用、常量、volatile，但是不包括指针特性
        constexpr auto size = std::tuple_size<typename std::decay<Tuple>::type>::value;
        return invoke_impl(std::forward<Function>(func), std::forward<Tuple>(t), std::make_index_sequence<size>{});
    }

    // 调用帮助类，主要用于返回是否void的情况, 为服务端调用
    template<typename R, typename F, typename ArgsTuple>
    // std::is_same判断两个传入的形参是否类型相同
    // std::enable_if: 当<>中的第一个参数返回true时，type的类型为第二个参数的类型
    typename std::enable_if<std::is_same<R, void>::value, typename type_xx<R>::type >::type
    call_helper(F f, ArgsTuple args) {
        invoke(f, args);
        return 0;
    }

    template<typename R, typename F, typename ArgsTuple>
    typename std::enable_if<!std::is_same<R, void>::value, typename type_xx<R>::type >::type
    call_helper(F f, ArgsTuple args) {
        return invoke(f, args);
    }

    // rpc传递的参数类型（包括rpc调用函数的错误码，错误信息和值包括要调用的rpc函数名，参数类型以及参数值）
    template<typename T>
    class value_t {
    public:
        typedef typename type_xx<T>::type type;
        typedef std::string msg_type;
        typedef uint16_t code_type;

        value_t() { _code = 0; _msg.clear(); }
        bool valid() { return _code == 0; }
        int error_code() { return _code; }
        std::string error_msg() { return _msg; }
        type val() { return _val; }

        void set_val(const type& val) { _val = val; }
        void set_code(code_type code) { _code = code; }
        void set_msg(msg_type msg) { _msg = msg; }

        // 反序列化
        friend Serializer& operator >> (Serializer& in, value_t<T>& d) {
            in >> d._code >> d._msg;
            if (d._code == 0) {
                in >> d._val;
            }
            return in;
        }
        // 序列化
        friend Serializer& operator << (Serializer& out, value_t<T> d) {
            out << d._code << d._msg << d._val;
            return out;
        }

    private:
        code_type _code;
        msg_type _msg;
        type _val;

        friend class RpcServer;
        friend class RpcClient;
    };
}