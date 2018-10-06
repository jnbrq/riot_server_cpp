/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 */

#ifndef RIOT_SERVER_SIMPLE_WS_INCLUDED
#define RIOT_SERVER_SIMPLE_WS_INCLUDED

#include <riot/server/connection_base.hpp>

#include <sstream>
#include <limits>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace riot::server {
namespace ws {
namespace detail {

using namespace boost::asio;
namespace websocket = boost::beast::websocket;

using error_code = boost::system::error_code;

#define KEEP_CONN   _ref = this->shared_from_this()

template <typename ConnectionManager>
struct connection: connection_base<ConnectionManager> {
    using connection_base_type = connection_base<ConnectionManager>;
    using base_type = connection_base_type;
    using typename connection_base<ConnectionManager>::write_mode_t;
    
    explicit connection(ConnectionManager &conn_man_):
        connection_base<ConnectionManager>{conn_man_},
        io_ctx_{conn_man_.io_context},
        ws_{io_ctx_},
        sock{ws_.next_layer()} {
        this->send_trailing_newline = false;
            // ^^^ message-based protocol?
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }
    
    void async_ws_start() {
        ws_.async_accept([this, KEEP_CONN] (const error_code &ec) {
            if (ec)
                return;
            this->async_start();
        });
    }
    
    virtual ~connection() {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        error_code ec;
        sock.shutdown(ip::tcp::socket::shutdown_both, ec);
    }
    
private:
    io_context &io_ctx_;
    boost::beast::multi_buffer buffer_;
    websocket::stream<ip::tcp::socket> ws_;
public:
    ip::tcp::socket &sock;

protected:
    void do_async_write(
        const char *data,
        std::size_t size,
        std::function<void (bool)> callback,
        write_mode_t write_mode) override {
        
        if (write_mode == write_mode_t::write_text) {
            ws_.text(true);
            ws_.binary(false);
        }
        else {
            ws_.text(false);
            ws_.binary(true);
        }
        
        ws_.async_write(
            boost::asio::buffer(data, size),
            [_f = std::move(callback)](const error_code &ec, std::size_t bt) {
                boost::ignore_unused(bt);
                if (ec) {
                    _f(false);
                    return ;
                }
                _f(true);
            }
        );
    }
    
    void do_async_read_message(
        std::function<void (const std::string &, bool)> callback) override {
        ws_.async_read(
            buffer_,
            [this, _f = std::move(callback)] (
                const error_code &ec, std::size_t bt) {
                boost::ignore_unused(bt);
                if (ec) {
                    _f("", false);
                    return;
                }
                std::ostringstream oss;
                oss << boost::beast::buffers(buffer_.data());
                buffer_.consume(buffer_.size()); // IDK, tho
                auto str = oss.str();
                _f(str, true);
            });
    }
    
    void do_set_async_read_message_max_size(std::size_t sz) override {
        ws_.read_message_max(sz);
            // 0 means maximum, as stated in Beast docs
    }
    
    void do_async_read_binary(
        char *buffer,
        std::size_t size,
        std::function<void (bool)> callback) override {
        ws_.async_read(
            buffer_,
            [this, buffer, size, _f = std::move(callback)] (
                const error_code &ec, std::size_t bt) {
                if (ec) {
                    _f(false);
                    return;
                }
                std::ostringstream oss;
                oss << boost::beast::buffers(buffer_.data());
                buffer_.consume(buffer_.size()); // IDK, tho
                auto str = oss.str();
                if (str.size() >= size) {
                    std::copy(str.begin(), str.begin() + size, buffer);
                }
                else {
                    // we have buffer underflow
                    /*
                    connection_base_type::send_text(
                        "err ",
                        "connection closed due to buffer underflow");
                    _f(false);
                    */
                    std::copy(str.begin(), str.end(), buffer);
                    _f(true);   // silently ignore this problem
                                // the rest of the buffer should be \0s
                    return ;
                }
                _f(true);
            });
    }
    
    void do_close() override {
        error_code ec;
        sock.close(ec);
    };
    
    void do_block_endpoint() override {
        // FIXME implement?
    }
};

#undef KEEP_CONN

// almost identical to simple TCP/IP server
template <typename ConnectionManager>
struct server {
    using connection_type = connection<ConnectionManager>;
    
    ConnectionManager &conn_man;
    
    server(
        ConnectionManager &conn_man_,
        const std::string &address = "0.0.0.0",
        unsigned short port = 8001,
        std::size_t id = std::numeric_limits<std::size_t>::max()):
        conn_man{conn_man_},
        io_ctx_{conn_man_.io_context},
        acceptor_{io_ctx_, ip::tcp::endpoint{
            ip::address::from_string(address), port}},
        id_{id} {
    }
    
    void async_start() {
        async_accept_connection();
    }
private:
    io_context &io_ctx_;
    ip::tcp::acceptor acceptor_;
    std::size_t id_;
    
    std::shared_ptr<connection_type> connection_;
    
    void async_accept_connection() {
        connection_ = std::make_shared<connection_type>(conn_man);
        connection_->server_id = id_;
        acceptor_.async_accept(
            connection_->sock, [this](const error_code &ec) {
            if (ec) {
                return ;
            }
            connection_->async_ws_start();
            async_accept_connection();
        });
    }
};

}

using detail::server;

}

}

#endif // RIOT_SERVER_SIMPLE_WS_INCLUDED
