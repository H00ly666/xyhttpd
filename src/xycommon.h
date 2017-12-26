#ifndef XYHTTPD_COMMON_H
#define XYHTTPD_COMMON_H

#include <memory>
#include <exception>

using namespace std;

#define XY_MESSAGE_REQUEST 1
#define XY_MESSAGE_STRING  2
#define XY_MESSAGE_CHUNK   3
#define XY_MESSAGE_WSFRAME 4
#define XY_MESSAGE_FCGI    5

class message {
public:
    virtual int type() const = 0;
    virtual ~message() = 0;
};

class streambuffer {
public:
    streambuffer();
    virtual void pull(int nbytes);
    virtual void *prepare(int nbytes);
    virtual void enlarge(int nbytes);
    virtual void append(void *buffer, int nbytes);
    inline int size() const { return _size; }
    inline char *data() const { return (char *)_data; };
    virtual ~streambuffer();
    static shared_ptr<streambuffer> alloc();
private:
    char *_data;
    int _size;

    streambuffer(const streambuffer &);
    streambuffer &operator=(const streambuffer &);
};

class decoder {
public:
    virtual bool decode(shared_ptr<streambuffer> &stb) = 0;
    virtual shared_ptr<message> msg() = 0;
    virtual ~decoder() = 0;
};

#endif