/**
 * @author Canberk Sönmez
 * 
 * @date Sat Jul 21 18:32:17 +03 2018
 * 
 * A simple TCP/IP and WebSockets servers using a customized connection
 * manager, created partially using make_connection_manager(...) and its
 * friends.
 * 
 */

#include <boost/asio.hpp>
#include <riot/server/servers/tcp_ip.hpp>
#include <riot/server/servers/ws.hpp>

// this header file contains make_connection_manager(...) and its friends
#include <riot/server/specialized/connection_manager.hpp>
#include <riot/mpl/filtered_overload.hpp>

#include <iostream>

int main(int argc, char **argv) {
    using namespace riot::server;
    
    // for make_connection_manager(...) and its friends
    using namespace riot::server::specialized;
    
    // for filter(...)
    using namespace riot::mpl;
    
    using namespace riot::server::artifacts;
    
    // this struct is embedded to each and every connection
    // it must be default-constructable
    struct data_type {
        // fill any connection-specific information here
        // you can access them using "conn.data"
    };
    
    boost::asio::io_context io_ctx;
    
    /**
     * In this situation we have two server, a TCP/IP server and a WebSockets
     * server (as usual). The TCP/IP server listens on localhost, it does not
     * accept any external connections. Therefore, any connection to it is
     * presumed to be safe, hence no password protection. On the other hand,
     * WebSockets server listens to all interfaces, potentially including
     * the Internet. Therefore, we want to provide some "password" protection
     * for it.
     * 
     * In this example, we will use specialized connection_manager to simply
     * implement this behaviour.
     */
    
    // server IDs for more complicated things
    enum: std::size_t {
        id_tcp_server,
        id_ws_server
    };
    
    connection_manager conn_man(
        // the Boost.ASIO infrastructure
        io_ctx,
        filtered_overload(
            filter(
                // this part is valid if and only if the connection is a ws
                // connection
                [](auto &conn, auto const &) {
                    return conn.server_id == id_ws_server;
                },
                case_of<can_activate>([](auto &conn, auto const &a) {
                    std::cout <<
                        "A ws connection tries to authenticate "
                        "checking for its password..." << "\n";
                    return conn.get_property_first("password").value_or("") == "1234";
                })
            ),
            filter(
                // this part is valid if and only if the connection is a raw
                // TCP/IP connection
                [](auto &conn, auto const &) {
                    return conn.server_id == id_tcp_server;
                },
                case_of<can_activate>([](auto &conn, auto const &) {
                    std::cout <<
                        "The number of connections right now: " <<
                        conn.conn_man.connections.size() << "\n";
                    return true;
                }),
                case_of<can_receive_event>([] (auto &conn, auto const &a) {
                    std::cout <<
                        "A TCP/IP connection tries to receive an event "
                        "the event name is: " << a.event->evt << "\n";
                    return true;
                })
            ),
            fallback::artifact_provider{}
        ),
        fallback::security_policy{},
        data_type{}
    );
    
    // usual TCP/IP server
    tcp_ip::server server1{
        conn_man,       // using the given connection manager
        "127.0.0.1",    // at all interfaces
        8000,           // the TCP port 8000
        id_tcp_server   // id of the server for connection identification
    };
    
    // WebSockets server (the parameters are the same)
    ws::server server2{
        conn_man,       // using the given connection manager
        "0.0.0.0",      // at all interfaces
        8001,           // the TCP port 8001
        id_ws_server    // id of the server for connection identification
    };
    
    // do not forget to start both servers at once
    server1.async_start();
    server2.async_start();
    
    io_ctx.run();
    return 0;
}
