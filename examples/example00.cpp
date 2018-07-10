/**
 * @author Canberk Sönmez
 * 
 * @date Wed Jul 11 01:21:08 +03 2018
 * 
 */

#include <boost/asio.hpp>
#include <riot/server/simple/connection_manager.hpp>
#include <riot/server/servers/tcp_ip.hpp>

int main(int argc, char **argv) {
    using namespace riot::server;
    
    // asynchronous I/O interface
    boost::asio::io_context io_ctx;
    
    // a simple connection manager to handle connections
    simple::connection_manager connection_manager{io_ctx};
    
    // listen for tcp/ip connections only
    tcp_ip::server<simple::connection_manager> server1{
        connection_manager, // using the given connection manager
        "127.0.0.1",        // only at the loopback address (0.0.0.0 for all)
        5000                // at port 5000
    };
    
    server1.async_start();
    io_ctx.run();
    
    return 0;
}
