#include <iostream>
#include <cstring>

#include "xyhttp.h"


#define CURRENT_ISUPPER (chunk[i] >= 'A' && chunk[i] <= 'Z')
#define CURRENT_ISLOWER (chunk[i] >= 'a' && chunk[i] <= 'z')
#define CURRENT_ISNUMBER (chunk[i] >= '0' && chunk[i] <= '9')
#define CURRENT_VALUE chunk + currentBase, i - currentBase

http_request::http_request() {}

void http_request::set_method(const char *method) {
    if(strcmp(method, "GET") == 0) _meth = GET;
    else if(strcmp(method, "POST") == 0) _meth = POST;
    else if(strcmp(method, "HEAD") == 0) _meth = HEAD;
    else if(strcmp(method, "PUT") == 0) _meth = PUT;
    else if(strcmp(method, "DELETE") == 0) _meth = DELETE;
    else if(strcmp(method, "OPTIONS") == 0) _meth = OPTIONS;
    else if(strcmp(method, "CONNECT") == 0) _meth = CONNECT;
    else if(strcmp(method, "BREW") == 0) _meth = BREW;
    else if(strcmp(method, "M-SEARCH") == 0) _meth = M_SEARCH;
    else
        throw invalid_argument("unsupported method");
}

int http_request::type() const {
    return XY_MESSAGE_REQUEST;
}

http_request::~http_request() {}

http_request::decoder::decoder() {}

bool http_request::decoder::decode(shared_ptr<streambuffer> &stb) {
    shared_ptr<http_request> req(new http_request());
    const char *chunk = stb->data();
    int i = 0, currentExpect = 0, currentBase;
    int verbOrKeyLength;
    char headerKey[32];
    if(stb->size() > 0x10000)
        throw runtime_error("request too long");
    while(i < stb->size()) {
        switch(currentExpect) {
            case 0: // expect HTTP method
                if(chunk[i] == ' ') {
                    verbOrKeyLength = i;
                    if(verbOrKeyLength > 20)
                        goto on_malformed_header;
                    currentBase = i + 1;
                    currentExpect = 1;
                }
                else if(!CURRENT_ISUPPER)
                    goto on_malformed_header;
                break;
            case 1: // expect resource
                if(chunk[i] == ' ') {
                    char method[32];
                    memcpy(method, chunk, verbOrKeyLength);
                    method[verbOrKeyLength] = 0;
                    req->set_method(method);
                    req->_resource = shared_ptr<string>(new string(CURRENT_VALUE));
                    currentBase = i + 1;
                    currentExpect = 2;
                }
                else if(chunk[i] >= 0 && chunk[i] < 32)
                    goto on_malformed_header;
                break;
            case 2: // expect HTTP version - HTTP/1.
                if(chunk[i] == '.') {
                    if(i - currentBase != 6 ||
                       chunk[currentBase + 0] != 'H' ||
                       chunk[currentBase + 1] != 'T' ||
                       chunk[currentBase + 2] != 'T' ||
                       chunk[currentBase + 3] != 'P' ||
                       chunk[currentBase + 4] != '/' ||
                       chunk[currentBase + 5] != '1') {
                        goto on_malformed_header;
                    }
                    currentBase = i + 1;
                    currentExpect = 3;
                }
                else if(!CURRENT_ISUPPER &&
                        chunk[i] != '/' && chunk[i] != '1') {
                    goto on_malformed_header;
                }
                break;
            case 3: // expect HTTP subversion
                if(chunk[i] == '\n') {
                    currentBase = i + 1;
                    currentExpect = 4;
                }
                else if(chunk[i] == '\r')
                    currentExpect = 100;
                else if((chunk[i] != '0' && chunk[i] != '1') ||
                        i != currentBase)
                    goto on_malformed_header;
                break;
            case 4: // expect HTTP header key
                if(chunk[i] == ':') {
                    verbOrKeyLength = i - currentBase;
                    currentExpect = 5;
                }
                else if(i == currentBase && chunk[i] == '\n')
                    goto entire_request_decoded;
                else if(i == currentBase && chunk[i] == '\r')
                    currentExpect = 101;
                else if(i - currentBase > 30)
                    goto on_malformed_header;
                else if(CURRENT_ISUPPER)
                    headerKey[i - currentBase] = chunk[i] + 32;
                else if(CURRENT_ISLOWER || chunk[i] == '-' || chunk[i] == '_' ||
                        CURRENT_ISNUMBER)
                    headerKey[i - currentBase] = chunk[i];
                else
                    goto on_malformed_header;
                break;
            case 5: // skip spaces between column and value
                if(chunk[i] != ' ') {
                    currentBase = i;
                    currentExpect = 6;
                }
                break;
            case 6: // expect HTTP header value
                if(!(chunk[i] >= 0 && chunk[i] < 32))
                    break;
                else if(chunk[i] == '\n' || chunk[i] == '\r') {
                    req->_headers[string(headerKey, verbOrKeyLength)]
                            = shared_ptr<string>(new string(CURRENT_VALUE));
                    currentBase = i + 1;
                    currentExpect = chunk[i] == '\r' ? 100 : 4;
                    break;
                }
                goto on_malformed_header;
            case 100: // expect \n after '\r'
                if(chunk[i] != '\n') goto on_malformed_header;
                currentBase = i + 1;
                currentExpect = 4;
                break;
            case 101: // expect final \n
                if(chunk[i] != '\n') goto on_malformed_header;
                goto entire_request_decoded;
        }
        i++;
    }
    return false;
    on_malformed_header:
    throw runtime_error("malformed request");
    entire_request_decoded:
    _msg = req;
    stb->pull(i + 1);
    return true;
}

shared_ptr<message> http_request::decoder::msg() {
    return _msg;
}

http_request::decoder::~decoder() {}

const char* http_response::state_description(int code) {
    switch(code) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Fobidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 408: return "Request Time-out";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 418: return "I'm a teapot";
        case 426: return "Upgrade Required";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return NULL;
    }
}

http_server::http_server(shared_ptr<http_service> svc) : service(svc) {
    stream = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
    if(uv_tcp_init(uv_default_loop(), stream) < 0) {
        free(stream);
        throw runtime_error("failed to initialize libuv TCP stream");
    }
    stream->data = this;
}

http_connection::http_connection(shared_ptr<http_service> svc, shared_ptr<stream> strm)
    : _reqdec(new http_request::decoder()), _svc(svc), _strm(strm),
      _keep_alive(true), _upgraded(false) {}

shared_ptr<http_request> http_connection::next_request() {
    _req = dynamic_pointer_cast<http_request>(_strm->read(_reqdec));
    shared_ptr<string> connhdr = _req->header("connection");
    if(connhdr) {
        _keep_alive = connhdr->find("keep-alive", 0, 10) != string::npos ||
                      connhdr->find("Keep-Alive", 0, 10) != string::npos;
    } else {
        _keep_alive = false;
    }
    return _req;
}

bool http_connection::keep_alive() {
    return _keep_alive;
}

http_server::http_server(http_service *svc)
        : http_server(shared_ptr<http_service>(svc)) {}

static void http_service_loop(void *data) {
    shared_ptr<http_connection> conn((http_connection *)data);
    while(conn->keep_alive()) {
        shared_ptr<http_request> req;
        try {
            req = conn->next_request();
        }
        catch(exception &ex) {
            break;
        }
        char resp[0x10000];
        sprintf(resp, "HTTP/1.1 200 OK\r\nServer: xyhttpd\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n%s",
                conn->keep_alive() ? "keep-alive" : "close",
                req->resource()->size(),
                req->resource()->c_str());
        conn->strm()->write(resp, strlen(resp));
    }
}

static void http_server_on_connection(uv_stream_t* strm, int status) {
    http_server *self = (http_server *)strm->data;
    if(status >= 0) {
        tcp_stream *client = new tcp_stream();
        if(client->accept(strm) < 0) {
            delete client;
            return;
        }
        shared_ptr<fiber> f = fiber::make(http_service_loop,
            new http_connection(self->service, shared_ptr<stream>(client)));
        f->resume();
    }
}

void http_server::listen(const char *addr, int port) {
    sockaddr_storage saddr;
    if(uv_ip4_addr(addr, port, (struct sockaddr_in*)&saddr) &&
       uv_ip6_addr(addr, port, (struct sockaddr_in6*)&saddr))
        throw invalid_argument("invalid IP address or port");
    int r = uv_tcp_bind(stream, (struct sockaddr *)&saddr, 0);
    if(r < 0)
        throw runtime_error(uv_strerror(r));
    r = uv_listen((uv_stream_t *)stream, 80, http_server_on_connection);
    if(r < 0)
        throw runtime_error(uv_strerror(r));
}

http_server::~http_server() {
    if(stream) {
        uv_close((uv_handle_t *)stream, (uv_close_cb) free);
    }
}