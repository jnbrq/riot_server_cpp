/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 * This example opens two servers at once, of different types. They serve
 * for TCP/IP connections and WebSockets connections.
 * 
 */

#include <boost/asio.hpp>
#include <riot/server/simple/connection_manager.hpp>
#include <riot/server/servers/tcp_ip.hpp>

// WebSockets server
#include <riot/server/servers/ws.hpp>

int main() {
    using namespace riot::server;
    
    boost::asio::io_context io_ctx;
    
    simple::connection_manager connection_manager{io_ctx};
    
    // usual TCP/IP server
    tcp_ip::server<simple::connection_manager> server1{
        connection_manager, // using the given connection manager
        "0.0.0.0",          // at all interfaces
        8000                // the TCP port 8001
    };
    
    // WebSockets server (the parameters are the same)
    ws::server<simple::connection_manager> server2{
        connection_manager, // using the given connection manager
        "0.0.0.0",          // at all interfaces
        8001                // the TCP port 8001
    };
    
    // do not forget to start both servers at once
    server1.async_start();
    server2.async_start();
    
    io_ctx.run();
    return 0;
}
