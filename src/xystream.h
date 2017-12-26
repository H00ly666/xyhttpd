#ifndef XYHTTPD_STREAM_H
#define XYHTTPD_STREAM_H

#include <uv.h>

#include "xycommon.h"
#include "xyfiber.h"

class int_status : public wakeup_event {
public:
    inline int_status(int s) : _status(s) {};
    inline int_status(const int_status &s)
        : _status(s._status) {}
    static inline shared_ptr<int_status> make(int s) {
        return shared_ptr<int_status>(new int_status(s));
    }
    inline int status() {
        return _status;
    }
    const char *strerror();
    const char *errname();
    virtual ~int_status();
private:
    int _status;
};

class stream {
public:
    shared_ptr<streambuffer> buffer;
    shared_ptr<fiber> reading_fiber, writing_fiber;

    virtual int accept(uv_stream_t *);
    virtual shared_ptr<message> read(shared_ptr<decoder>);
    virtual void write(char *buf, int length);
    virtual ~stream();
protected:
    uv_stream_t *handle;
    stream();
private:
    stream(const stream &);
};

class tcp_stream : public stream {
public:
    tcp_stream();
};

class string_message : public message {
public:
    explicit string_message(shared_ptr<string> str);
    string_message(const string_message &);
    inline shared_ptr<string> str() {
        return _str;
    }
    virtual int type() const;
    virtual ~string_message();
private:
    shared_ptr<string> _str;
};

class string_decoder : public decoder {
public:
    virtual bool decode(shared_ptr<streambuffer> &stb);
    virtual shared_ptr<message> msg();
    virtual ~string_decoder();
private:
    shared_ptr<string> _str;
};

#endif
