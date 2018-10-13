/**
 * @author Canberk Sönmez
 * 
 * @date Wed Jul 11 01:21:08 +03 2018
 * 
 * A simple TCP/IP and WebSockets servers using a customized connection
 * manager.
 * 
 */

#include <boost/asio.hpp>

#include <riot/server/servers/tcp_ip.hpp>
#include <riot/server/servers/ws.hpp>

#include <utility>
#include <list>
#include <map>

namespace my_server {

namespace detail {

using namespace riot;
using namespace riot::server;
using namespace riot::server::security_actions;
using namespace riot::server::artifacts;

/**
 * @brief Custom security_policy class.
 * 
 * The possible security actions can be found in security_actions.hpp
 */
struct security_policy {
    // say the default policy is to simply raising warnings
    template <typename ConnectionBase, typename SecurityAction>
    auto operator()(ConnectionBase &, SecurityAction) {
        return action_raise_warning_and_ignore;
    }
    
    // in case of a DoS attack, the connection should be halted
    template <typename ConnectionBase>
    auto operator()(ConnectionBase &, header_size_limit_reached<ConnectionBase>) {
        // please note that one can hold a list of endpoints
        // and count how frequent this happens and decide accordingly
        return action_raise_error_and_halt;
    }
};

/**
 * @brief Custom artifact_provider class.
 * 
 * The possible artifacts can be found in artifacts.hpp
 */
struct artifact_provider {
    template <typename ConnectionBase>
    std::size_t operator()(ConnectionBase &, header_message_max_size<ConnectionBase>) {
        // a header message can be at most 50 bytes (with \n)
        // this is thought as a DoS protection
        return 50;
    }
    
    template <typename ConnectionBase>
    std::size_t operator()(ConnectionBase &, header_max_size<ConnectionBase>) {
        // the total size of the header can be at most 200
        return 200;
    }
    
    template <typename ConnectionBase>
    bool operator()(ConnectionBase &conn, can_activate<ConnectionBase>) {
        // say a connection can activate only if it has a property called
            // password and it is 1234
        //
        // actual security can be implemented here
        
        // please note that properties has a type of:
        // std::map<std::string, std::list<std::string>>
        // you can see en.cppreference.com for std::map<>::find()
        
        auto it = conn.properties.find("password");
        if (it != conn.properties.end()) {
            // if password exists
            if (it->second.front() == "1234") {
                // due to the header grammar at least one element is 
                // guaranteed to exist, if password exists
                return true;
            }
        }
        return false;
    }
    
    template <typename ConnectionBase>
    duration_t operator()(
        ConnectionBase &, minimum_time_between_triggers<ConnectionBase>) {
        // this can be used to dictate a minimum sampling period so that
        // the network is not overwhelmed
        return 500;
    }
    
    template <typename ConnectionBase>
    bool operator()(ConnectionBase &, can_execute_code<ConnectionBase>) {
        // this is not used as code execution is not implemented yet
        // however, it is reserved
        return false;
    }
    
    template <typename ConnectionBase>
    duration_t operator()(ConnectionBase &, freeze_duration<ConnectionBase>) {
        // as a result of the security action, the connection might be frozen
        // this is how long it should be frozen
        return 5000;
    }
    
    template <typename ConnectionBase>
    bool operator()(
        ConnectionBase & conn,
        can_receive_event<ConnectionBase> s) {
        // say dev1 cannot receive events from dev2, for example
        if (conn.name == "dev1" && s.event->sender->name == "dev2")
            return false;
        return true;
    }
    
    template <typename ConnectionBase>
    bool operator()(ConnectionBase &conn, can_trigger_event<ConnectionBase> s) {
        // say dev1 cannot trigger EVT_TEST
        if (conn.name == "dev1" && s.evt == "EVT_TEST")
            return false;
        return true;
    }
    
    template <typename ConnectionBase>
    duration_t operator()(ConnectionBase &, keep_alive_period<ConnectionBase>) {
        return 1000 * 60 * 60 * 24 * 7;   // 1 week
    }
};
}

/**
 * @brief Custom connection_manager class.
 * 
 * There are a few requirement of the ConnectionManager concept. For a
 * ConnectionManager CM, the followings should be satisfied:
 * 
 *      - CM.security_policy(ConnectionBase, SecurityAction) is valid
 *        and returns a valid security_actions::action
 * 
 *      - CM.artifact_provider(ConnectionBase, Artifact) is valid
 *        and its return value is compatible with Artifact::result_type.
 * 
 *      - CM.io_context is a valid Boost.ASIO io_context object or a reference
 *        to a valid instance.
 * 
 *      - CM.connections is of type:
 *        std::list<std::weak_ptr<ConnectionBase>> (or alike)
 * 
 */
struct connection_manager {
    // the constructor should bind the io_ctx reference to an io_context
    // object. another alternative might be defining io_ctx as an object.
    connection_manager(boost::asio::io_context &io_ctx_): io_context{io_ctx_} {
    }

    // necessary (1/4)
    detail::security_policy security_policy;
    
    // necessary (2/4)
    detail::artifact_provider artifact_provider;
    
    // necessary (3/4)
    std::list<std::weak_ptr<ConnectionBase>> connections;
    
    // necessary (4/4)
    boost::asio::io_context &io_context;
};

}

int main(int argc, char **argv) {
    using namespace riot::server;
    
    boost::asio::io_context io_ctx;
    
    my_server::connection_manager connection_manager{io_ctx};
    
    // usual TCP/IP server
    tcp_ip::server server1{
        connection_manager, // using the given connection manager
        "0.0.0.0",          // at all interfaces
        8000                // the TCP port 8000
    };
    
    // WebSockets server (the parameters are the same)
    ws::server server2{
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
