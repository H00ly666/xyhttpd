#ifndef XYHTTPD_HTTP_H
#define XYHTTPD_HTTP_H

#include "xycommon.h"
#include "xyfiber.h"
#include "xystream.h"

#include <map>
#include <uv.h>

enum http_method {
    GET, HEAD, POST, PUT, DELETE, OPTIONS,
    CONNECT, // HTTP Tunnelling
    BREW, // Hypertext Coffee Pot Control Protocol
    M_SEARCH, // Universal PnP Multicast Search
};

class http_request : public message {
public:
    http_request();
    inline http_method method();
    inline shared_ptr<string> resource() { return _resource; }
    inline shared_ptr<string> path() { return _path; }
    inline shared_ptr<string> query() { return _query; }
    inline shared_ptr<string> header(const string &key) {
        return _headers.at(key);
    }
    virtual int type() const;
    virtual ~http_request();

    class decoder : public ::decoder {
    public:
        decoder();
        virtual bool decode(shared_ptr<streambuffer> &stb);
        virtual shared_ptr<message> msg();
        virtual ~decoder();
    private:
        shared_ptr<http_request> _msg;

        decoder(const decoder &);
        decoder &operator=(const decoder &);
    };
private:
    http_method _meth;
    shared_ptr<string> _resource;
    shared_ptr<string> _path;
    shared_ptr<string> _query;
    map<string, shared_ptr<string>> _headers;

    http_request(const http_request &);
    http_request &operator=(const http_request &);

    void set_method(const char *method);
};

class http_response : public message {
public:
    inline int code() { return _code; }
    inline shared_ptr<string> operator[](string &key) {
        return _headers.at(key);
    }
    virtual ~http_response();
    static const char *state_description(int code);
private:
    int _code;
    map<string, shared_ptr<string>> _headers;

    http_response(const http_response &);
    http_response &operator=(const http_response &);
};

class http_service {
    
};

class http_connection {
public:
    http_connection(shared_ptr<http_service> svc, shared_ptr<stream> strm);
    shared_ptr<http_request> next_request();
    bool keep_alive();
    inline shared_ptr<stream> strm() {
        return _strm;
    }
private:
    bool _keep_alive;
    bool _upgraded;
    shared_ptr<stream> _strm;
    shared_ptr<http_service> _svc;
    shared_ptr<http_request> _req;
    shared_ptr<http_request::decoder> _reqdec;
};

class http_server {
public:
    shared_ptr<http_service> service;
    http_server(shared_ptr<http_service> svc);
    http_server(http_service *svc);
    virtual ~http_server();

    void listen(const char *addr, int port = 80);
private:
    uv_tcp_t *stream;

    http_server(const http_server &);
    http_server &operator=(const http_server &);
};

#endif
