/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 */

#ifndef RIOT_SERVER_SIMPLE_TCP_IP_INCLUDED
#define RIOT_SERVER_SIMPLE_TCP_IP_INCLUDED

#include <iostream>
#include <typeinfo>
#include <type_traits>
#include <list>
#include <memory>
#include <boost/core/ignore_unused.hpp>
#include <riot/server/connection_base.hpp>

namespace riot::server {
namespace simple_tcp_ip {
namespace detail {

template <typename ConnectionManager>
struct connection;

template <typename ConnectionManager>
struct server;

using namespace boost::asio;
using error_code = boost::system::error_code;

template <typename ConnectionManager>
struct connection: connection_base<ConnectionManager> {
    using typename connection_base<ConnectionManager>::write_mode_t;
    
    explicit connection(ConnectionManager &conn_man_):
        connection_base<ConnectionManager>{conn_man_},
        io_ctx_{conn_man_.io_ctx},
        sock{io_ctx_} {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }
    
    virtual ~connection() {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }
    
private:
    io_context &io_ctx_;
    streambuf buffer_;
public:
    ip::tcp::socket sock;
protected:
    void do_async_write(
        const char *data,
        std::size_t size,
        std::function<void (bool)> callback,
        write_mode_t /* write_mode */) override {
        async_write(
            sock,
            buffer(data, size),
            [_f = std::move(callback)](const error_code &ec, std::size_t bt) {
                boost::ignore_unused(bt);
                if (ec) {
                    _f(false);
                    return ;
                }
                _f(true);
            });
    }
    
    void do_set_async_read_message_max_size(std::size_t sz) override {
        
    }
    
    void do_async_read_message(
        std::function<void (const std::string &, bool)> callback) override {
        async_read_until(
            sock,
            buffer_,
            '\n',
            [this, _f = std::move(callback)](
                const error_code &ec, std::size_t bt) {
                boost::ignore_unused(bt);
                if (ec) {
                    _f("", false);
                    return ;
                }
                std::istream is(&buffer_);
                std::string msg;
                std::getline(is, msg);
                _f(msg, true);
            });
    }
    
    void do_async_read_binary(
        char *buffer,
        std::size_t size,
        std::function<void (bool)> callback) override {
        /* it is always possible to have some leaking bytes in buffer_,
         * so, first, check if we can read size bytes from the buffer_ */
        std::istream is(&buffer_);
        is.read(buffer, size);
        auto already_read = is.gcount();
        if (already_read >= size) {
            // the handler should not be executed immediately
            // post it to the io_ctx to be executed later
            // this way, we simulate the async_read behaviour
            post(io_ctx_, [_f = std::move(callback)]() {
                _f(true);
            });
        }
        else {
            async_read(
                sock,
                boost::asio::buffer(
                    buffer + already_read, size - already_read),
                [_f = std::move(callback)](const error_code &ec, std::size_t) {
                    if (ec) {
                        _f(false);
                        return ;
                    }
                    _f(true);
                });
        }
    }
    
    void do_close() override {
        error_code ec;
        sock.close(ec);
    };
    
    void do_block_endpoint() override {
        // FIXME implement?
    }
};

template <typename ConnectionManager>
struct server {
    using connection_type = connection<ConnectionManager>;
    
    ConnectionManager &conn_man;
    
    explicit server(ConnectionManager &conn_man_):
        conn_man{conn_man_},
        io_ctx_{conn_man_.io_ctx},
        acceptor_{io_ctx_, ip::tcp::endpoint{ip::tcp::v4(), 8000}} {
    }
    
    void async_start() {
        async_accept_connection();
    }
private:
    io_context &io_ctx_;
    ip::tcp::acceptor acceptor_;
    
    std::shared_ptr<connection_type> connection_;
    
    void async_accept_connection() {
        connection_ = std::make_shared<connection_type>(conn_man);
        acceptor_.async_accept(
            connection_->sock, [this](const error_code &ec) {
            if (ec) {
                return ;
            }
            connection_->async_start();
            async_accept_connection();
        });
    }
};

}

using detail::server;
}

}

#endif // RIOT_SERVER_SIMPLE_TCP_IP_INCLUDED