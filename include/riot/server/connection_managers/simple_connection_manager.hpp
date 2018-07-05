/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 */

#ifndef RIOT_SERVER_SIMPLE_CONNECTION_MANAGER_INCLUDED
#define RIOT_SERVER_SIMPLE_CONNECTION_MANAGER_INCLUDED

#include <iostream>
#include <type_traits>
#include <list>
#include <memory>
#include <riot/server/security_actions.hpp>
#include <riot/server/server_artifacts.hpp>

namespace riot::server {

struct simple_security_policy {
    template <typename T>
    static void debug() {
        std::cout << "security_action: " << typeid(T).name() << "\n";
    }
    
    template <typename ConnectionBase, typename T>
    auto operator()(ConnectionBase &, T const &) {
        debug<T>();
        return security_actions::action_raise_warning_and_ignore;
    }
};

struct simple_artifact_provider {
    template <typename T>
    static void debug() {
        std::cout << "artifact: " << typeid(T).name() << "\n";
    }
    
    template <typename ConnectionBase, typename T>
    std::enable_if_t<
            std::is_same_v<typename T::result_type, bool>,
            typename T::result_type
        > operator()(ConnectionBase &, T const &) {
        debug<T>();
        return true;
    }
    
    template <typename ConnectionBase, typename T>
    std::enable_if_t<
            !std::is_same_v<typename T::result_type, bool>,
            typename T::result_type
        > operator()(ConnectionBase &, T const &) {
        debug<T>();
        return typename T::result_type(0);
    }
    
    template <typename ConnectionBase>
    std::size_t operator()(
        ConnectionBase &, server_artifacts::description_message_max_size &) {
        return 300;
    }
};

struct simple_connection_manager {
public:
    using connection_base_type = connection_base<simple_connection_manager>;
    
    simple_security_policy security_policy;
    simple_artifact_provider artifact_provider;
    std::list<std::weak_ptr<connection_base_type>> connections;
    
    // now the Boost-specific design
    boost::asio::io_context io_ctx;
    std::unique_ptr<boost::asio::io_context::work> work;
    
    simple_connection_manager() {
        work = std::make_unique<boost::asio::io_context::work>(io_ctx);
    }
};

}

#endif // RIOT_SERVER_SIMPLE_CONNECTION_MANAGER_INCLUDED
