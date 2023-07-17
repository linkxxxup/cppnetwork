#pragma once

#include <vector>
#include <algorithm>
#include <string.h>

namespace wut::zgy::cppnetwork {

class StreamBuffer : public std::vector<char> {
public:
    StreamBuffer() { _curpos = 0; }
    StreamBuffer(const char *in, size_t len) {
        _curpos = 0;
        insert(begin(), in, in + len);
    }
    ~StreamBuffer() {}

    void reset() { _curpos = 0; }
    const char *data() { return &(*this)[0]; }
    const char *current() { return &(*this)[_curpos]; }
    void offset(int k) { _curpos += k; }
    bool is_eof() { return (_curpos >= size()); }
    void input(char *in, size_t len) { insert(end(), in, in + len); }
    int findc(char c) {
        auto it = find(begin() + _curpos, end(), c);
        if (it != end()) {
            return it - (begin() + _curpos);
        }
        return -1;
    }
private:
    // 当前字节流位置
    unsigned int _curpos;
};

class Serializer {
public:
    Serializer() { _byteorder = LittleEndian; }
    ~Serializer() {}
    Serializer(StreamBuffer dev, int byteorder = LittleEndian) {
        _byteorder = byteorder;
        _iodevice = dev;
    }
public:
    enum ByteOrder {  //字节序
        BigEndian,
        LittleEndian
    };
public:
    void reset() {
        _iodevice.reset();
    }

    size_t size() {
        return _iodevice.size();
    }
    void skip_raw_date(int k) {
        _iodevice.offset(k);
    }
    const char *data() {
        return _iodevice.data();
    }
    void byte_reverse(char *in, int len) {
        if (_byteorder == BigEndian) {
            std::reverse(in, in + len);
        }
    }
    void write_raw_data(char *in, int len) {
        _iodevice.input(in, len);
        _iodevice.offset(len);
    }
    const char *current() {
        return _iodevice.current();
    }
    void clear() {
        _iodevice.clear();
        reset();
    }

    template<typename T>
    void output_type(T &t);
    template<typename T>
    void input_type(T t);

    // 直接给一个长度， 返回当前位置以后x个字节数据
    void get_length_mem(char *p, int len) {
        memcpy(p, _iodevice.current(), len);
        _iodevice.offset(len);
    }

public:
    template<typename Tuple, std::size_t Id>
    void getv(Serializer &ds, Tuple &t) {
        //这里重写了>>，不是字符串遇到空字符停止输出，而是根据类型所占的字节长度，将序列对象的数据输出
        ds >> std::get<Id>(t);
    }
    template<typename Tuple, std::size_t... I>
    Tuple get_tuple(std::index_sequence<I...>) {
        Tuple t;
        std::initializer_list<int> {((getv<Tuple, I>(*this, t)), 0)...};
        return t;
    }

    template<typename T>
    Serializer &operator>>(T &i) {
        output_type(i);
        return *this;
    }
    template<typename T>
    Serializer &operator<<(T i) {
        input_type(i);
        return *this;
    }

private:
    int _byteorder;
    StreamBuffer _iodevice;
//
    friend class Registry;
};

//如果不是char*和string类型，是基础类型，则直接按基础类型的大小输出值
//如果是char*或string类型，则先根据string对象的大小，输出string对象的值
template<typename T>
inline void Serializer::output_type(T &t) {
    int len = sizeof(T);
    char *d = new char[len];
    if (!_iodevice.is_eof()) {
        memcpy(d, _iodevice.current(), len);
        _iodevice.offset(len);
        byte_reverse(d, len);
        t = *reinterpret_cast<T *>(&d[0]);
    }
    delete[] d;
}
template<>
inline void Serializer::output_type(std::string &in) {
    int marklen = sizeof(uint16_t);
    char *d = new char[marklen];
    memcpy(d, _iodevice.current(), marklen);
    byte_reverse(d, marklen);
    int len = *reinterpret_cast<uint16_t *>(&d[0]);
    _iodevice.offset(marklen);
    delete[] d;
    if (len == 0) {
        LOG_ERROR("func_name is empty!")
        return;
    }
    in.insert(in.begin(), _iodevice.current(), _iodevice.current() + len);
    _iodevice.offset(len);
}

//如果不是char*和string类型，是基础类型，则直接按基础类型的大小存储值
//如果是char*或string类型，则先存储string对象的大小，再存储string对象的值
template<typename T>
inline void Serializer::input_type(T t) {
    int len = sizeof(T);
    char *d = new char[len];
    const char *p = reinterpret_cast<const char *>(&t);
    memcpy(d, p, len);
    byte_reverse(d, len);
    _iodevice.input(d, len);
    delete[] d;
}
template<>
inline void Serializer::input_type(std::string in) {
    // 先存入字符串长度
    uint16_t len = in.size();
    char *p = reinterpret_cast< char *>(&len);
    byte_reverse(p, sizeof(uint16_t));
    _iodevice.input(p, sizeof(uint16_t));
    // 存入字符串
    if (len == 0) return;
    char *d = new char[len];
    memcpy(d, in.c_str(), len);
    _iodevice.input(d, len);
    delete[] d;
}

template<>
inline void Serializer::input_type(const char *in) {
    input_type<std::string>(std::string(in));
}
}