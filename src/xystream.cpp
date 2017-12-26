#include "xystream.h"

const char *int_status::strerror() {
    return uv_strerror(_status);
}

int_status::~int_status() {}

stream::stream() {
    buffer = shared_ptr<streambuffer>(new streambuffer());
}

int stream::accept(uv_stream_t *svr) {
    return uv_accept(svr, handle);
}

static void stream_on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    stream *self = (stream *)handle->data;
    buf->base = (char *)self->buffer->prepare(suggested_size);
    buf->len = buf->base ? suggested_size : 0;
}

static void stream_on_data(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
    stream *self = (stream *)handle->data;
    if(nread > 0) self->buffer->enlarge(nread);
    self->reading_fiber->event = int_status::make(nread);
    self->reading_fiber->resume();
}

shared_ptr<message> stream::read(shared_ptr<decoder> decoder) {
    if(reading_fiber)
        throw runtime_error("reading from a stream occupied by another fiber");
    if(buffer->size() > 0)
        if(decoder->decode(buffer))
            return decoder->msg();
    if(!fiber::in_fiber())
        throw logic_error("reading from a stream outside a fiber");
    int r;
    if((r = uv_read_start(handle, stream_on_alloc, stream_on_data)) < 0)
        throw runtime_error(uv_strerror(r));
    reading_fiber = fiber::running();
    while(true) {
        shared_ptr<int_status> s = dynamic_pointer_cast<int_status>(fiber::yield());
        if(s->status() > 0) {
            try {
                if(decoder->decode(buffer)) {
                    uv_read_stop(handle);
                    reading_fiber.reset();
                    return decoder->msg();
                }
            }
            catch(exception &ex) {
                uv_read_stop(handle);
                reading_fiber.reset();
                throw ex;
            }
        }
        else {
            uv_read_stop(handle);
            reading_fiber.reset();
            throw runtime_error(s->strerror());
        }
    }
}

static void stream_on_write(uv_write_t *req, int status)
{
    stream *self = (stream *)req->data;
    delete req;
    self->writing_fiber->event = int_status::make(status);
    self->writing_fiber->resume();
}

void stream::write(char *chunk, int length)
{
    if(reading_fiber)
        throw runtime_error("writing to a stream occupied by another fiber");
    int r;
    uv_buf_t buf;
    buf.base = chunk;
    buf.len = length;
    uv_write_t *req = new uv_write_t;
    req->data = this;
    if((r = uv_write(req, handle, &buf, 1, stream_on_write)) < 0)
        throw runtime_error(uv_strerror(r));
    writing_fiber = fiber::running();
    shared_ptr<int_status> s = dynamic_pointer_cast<int_status>(fiber::yield());
    writing_fiber.reset();
    if(s->status() != 0)
        throw runtime_error(s->strerror());
}

stream::~stream() {
    if(handle) {
        uv_close((uv_handle_t *)handle, (uv_close_cb) free);
    }
}

tcp_stream::tcp_stream() {
    uv_tcp_t *h = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
    if(uv_tcp_init(uv_default_loop(), h) < 0) {
        free(h);
        throw runtime_error("failed to initialize libuv TCP stream");
    }
    handle = (uv_stream_t *)h;
    handle->data = this;
}

string_message::string_message(shared_ptr<string> s)
        : _str(s) {}


string_message::string_message(const string_message &s)
        : _str(s._str) {}

int string_message::type() const {
    return XY_MESSAGE_STRING;
}

string_message::~string_message() {}

shared_ptr<message> string_decoder::msg() {
    return shared_ptr<string_message>(new string_message(_str));
}

bool string_decoder::decode(shared_ptr<streambuffer> &stb) {
    _str = shared_ptr<string>(new string(stb->data(), stb->size()));
    stb->pull(stb->size());
    return true;
}

string_decoder::~string_decoder() {}