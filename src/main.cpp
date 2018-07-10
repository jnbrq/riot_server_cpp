/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 */

#include <iostream>
#include <thread>

#include <sol/sol.hpp>
#include <riot/server/simple/simple.hpp>

namespace riot::server {

namespace lua {

namespace detail {

class security_policy {

};

class artifact_provider {

};

}

class connection_manager {
    using connection_base_type = connection_base<connection_manager>;
    
    detail::security_policy security_policy;
    detail::artifact_provider artifact_provider;
    std::list<std::weak_ptr<connection_base_type>> connections;
};

}}

int main() {
    using namespace riot::server::simple;
    
    connection_manager conn_man;
    tcp_ip::server<connection_manager> server1(conn_man);
    ws::server<connection_manager> server2(conn_man);
    
    server1.async_start();
    server2.async_start();
    
    std::thread t{[&] {conn_man.io_ctx.run();}};
    std::cin.get();
    conn_man.io_ctx.stop();
    t.join();
    return 0;
}
