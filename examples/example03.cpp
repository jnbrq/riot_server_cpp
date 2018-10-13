/**
 * @author Canberk Sönmez
 * 
 * @date Wed Jul 11 17:31:39 +03 2018
 * 
 * In this example, the server takes an authentication token from an external
 * connection. This connection is purely local, it might be a connection to a
 * web server, for example. The result is to provide a convenient way to
 * authenticate and to integrate with web servers.
 * 
 * Of course, as the rest of this project, it is designed to be single
 * threaded.
 * 
 */

#include <boost/asio.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <memory>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <iostream>
#include <utility>
#include <map>

#include <riot/server/connection_base.hpp>
#include <riot/server/servers/tcp_ip.hpp>
#include <riot/server/servers/ws.hpp>

namespace external_auth {

namespace detail {

using namespace boost::asio;
using error_code = boost::system::error_code;

struct connection;
struct server;

struct connection: public std::enable_shared_from_this<connection> {
    detail::server &server;
    ip::tcp::socket sock;
    
    connection(struct server &s);
    
    void async_start() {
        read_next_line();
    }
private:
    boost::asio::streambuf buffer_;
    
    void read_next_line();
};

struct server {
    io_context &io_ctx;
    std::function<void (std::string, std::string)> add_auth_func;
    
    template <typename F>
    server(
        io_context &io_ctx_,
        F &&add_auth_func_,
        unsigned short port = 13587,
        const std::string &address = "127.0.0.1"
            /* last thing you's change */):
        io_ctx{io_ctx_},
        add_auth_func{std::forward<F>(add_auth_func_)},
        acceptor_{
            io_ctx_,
            ip::tcp::endpoint {
                ip::address::from_string(address),
                port
            }
        } {
    }
    
    void async_start() {
        async_accept_connection();
    }
private:
    ip::tcp::acceptor acceptor_;
    
    std::shared_ptr<connection> connection_;
    
    void async_accept_connection() {
        connection_ = std::make_shared<connection>(*this);
        acceptor_.async_accept(
            connection_->sock,
            [this](const error_code &ec) {
            if (ec)
                return ;
            
            connection_->async_start();
            async_accept_connection();
        });
    }
};

connection::connection(struct server& s):
    server{s},
    sock{server.io_ctx}
{  }

void connection::read_next_line()
{
    async_read_until(
        sock,
        buffer_,
        '\n',
        [this, _ref = shared_from_this()]
        (const error_code &ec, std::size_t) {
            if (ec)
                return;
            
            std::istream is{&buffer_};
            std::string line;
            std::getline(is, line);
            
            std::vector<std::string> toks;
            boost::algorithm::split(
                toks,
                line,
                boost::algorithm::is_any_of(" "),
                boost::algorithm::token_compress_on);
            
            if (toks.size() == 2) {
                if (this->server.add_auth_func)
                    this->server.add_auth_func(
                        std::move(toks[0]),
                        std::move(toks[1]));
            }
            
            read_next_line();
        });
}


}

using detail::server;

}



namespace my_server {

namespace detail {

using namespace riot;
using namespace riot::server;
using namespace riot::server::security_actions;
using namespace riot::server::artifacts;

struct security_policy {
    template <typename ConnectionBase, typename SecurityAction>
    auto operator()(ConnectionBase &, SecurityAction) {
        return action_raise_warning_and_ignore;
    }
    
    template <typename ConnectionBase>
    auto operator()(ConnectionBase &, header_size_limit_reached<ConnectionBase>) {
        return action_raise_error_and_halt;
    }
};

struct artifact_provider {
    std::map<std::string, std::string> auth_keys;
    
    template <typename ConnectionBase>
    std::size_t operator()(ConnectionBase &, header_message_max_size<ConnectionBase>) {
        return 50;
    }
    
    template <typename ConnectionBase>
    std::size_t operator()(ConnectionBase &, header_max_size<ConnectionBase>) {
        return 200;
    }
    
    template <typename ConnectionBase>
    bool operator()(ConnectionBase &conn, can_activate<ConnectionBase>) {
        // first, find the auth key
        auto it = conn.properties.find("auth_key");
        if (it != conn.properties.end()) {
            auto auth_key = it->second.front();
            // now, see if name, auth_key matches an object in our map
            auto it2 = auth_keys.find(conn.name);
            if (it2 != auth_keys.end()) {
                return it2->second == auth_key;
            }
        }
        return false;
    }
    
    template <typename ConnectionBase>
    duration_t operator()(
        ConnectionBase &, minimum_time_between_triggers<ConnectionBase>) {
        return 500;
    }
    
    template <typename ConnectionBase>
    bool operator()(ConnectionBase &, can_execute_code<ConnectionBase>) {
        return false;
    }
    
    template <typename ConnectionBase>
    duration_t operator()(ConnectionBase &, freeze_duration<ConnectionBase>) {
        return 5000;
    }
    
    template <typename ConnectionBase>
    bool operator()(
        ConnectionBase & conn,
        can_receive_event<ConnectionBase> s) {
        return true;
    }
    
    template <typename ConnectionBase>
    bool operator()(ConnectionBase &conn, can_trigger_event<ConnectionBase> s) {
        // say dev1 cannot trigger EVT_TEST
        return true;
    }
    
    template <typename ConnectionBase>
    duration_t operator()(ConnectionBase &, keep_alive_period<ConnectionBase>) {
        return 1000 * 60 * 60 * 24 * 7;   // 1 week
    }
};
}

struct connection_manager {
    using connection_base_type = riot::server::connection_base<connection_manager>;

    connection_manager(boost::asio::io_context &io_ctx_): io_context{io_ctx_} {
    }

    detail::security_policy security_policy;
    detail::artifact_provider artifact_provider;
    std::list<std::weak_ptr<connection_base_type>> connections;
    boost::asio::io_context &io_context;
};

}

int main() {
    using namespace riot::server;
    
    boost::asio::io_context io_ctx;
    
    my_server::connection_manager connection_manager{io_ctx};
    tcp_ip::server server1{
        connection_manager,
        "0.0.0.0",
        8000
    };
    
    ws::server server2{
        connection_manager,
        "0.0.0.0",
        8001
    };
    
    server1.async_start();
    server2.async_start();
    
    // start the external auth server
    external_auth::server server3{
        io_ctx,
        [&](std::string dev_name, std::string auth_key) {
            std::cout << "Adding auth_key: " << dev_name
                << " , " << auth_key << std::endl;
            
            connection_manager.artifact_provider.auth_keys[dev_name]
                = auth_key;
            
            // a deadline timer to remove it
            auto timer = std::make_shared<boost::asio::deadline_timer>(
                io_ctx);
            
            // expires in 5 seconds
            timer->expires_from_now(boost::posix_time::seconds(5));
            
            // and wait until expires
            timer->async_wait(
                [timer /* hold reference */, dev_name, &connection_manager]
                (const boost::system::error_code &ec) {
                if (ec)
                    return;
                auto &auth_keys = connection_manager.
                    artifact_provider.auth_keys;
                auto it = auth_keys.find(dev_name);
                if (it != auth_keys.end()) {
                    std::cout
                        << "Expired auth_key: " << it->first
                        << " , " << it->second << std::endl;
                    connection_manager.artifact_provider.auth_keys.erase(it);
                }
            });
        },
        7999
    };
    server3.async_start();
    
    io_ctx.run();
    return 0;
}
