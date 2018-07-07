/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 */

#include <iostream>

#include <thread>

#include <riot/server/servers/simple_ws.hpp>
#include <riot/server/servers/simple_tcp_ip.hpp>
#include <riot/server/connection_managers/simple_connection_manager.hpp>

int main() {
    using namespace riot::server;
    simple_connection_manager conn_man;
    simple_tcp_ip::server<simple_connection_manager> server1(conn_man);
    simple_ws::server<simple_connection_manager> server2(conn_man);
    
    server1.async_start();
    server2.async_start();
    
    std::thread t{[&] {conn_man.io_ctx.run();}};
    std::cin.get();
    conn_man.io_ctx.stop();
    t.join();
    return 0;
}
