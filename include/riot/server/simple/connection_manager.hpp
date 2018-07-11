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
#include <riot/server/connection_base.hpp>
#include <riot/server/security_actions.hpp>
#include <riot/server/artifacts.hpp>

namespace riot::server::simple {

struct security_policy {
    template <typename T>
    static void debug() {
        std::cout << "security_action: " << typeid(T).name() << "\n";
    }
    
    template <typename ConnectionBase, typename T>
    auto operator()(ConnectionBase &, T const &) {
        debug<T>();
        return security_actions::action_raise_warning_and_ignore;
    }
    
    template <typename ConnectionBase>
    auto operator()(
        ConnectionBase &,
        security_actions::header_size_limit_reached const&) {
        // this should be a must
        return security_actions::action_raise_error_and_halt;
    }
};

struct artifact_provider {
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
        ConnectionBase &,
        artifacts::header_message_max_size const &a) {
        /* const is necessary, otherwise it fails to match this one for
         * rvalue references !!! */
        // one could also define template <C, T> ... T & instead of
        // template <C, T> ... T const & .
        debug<std::decay_t<decltype(a)>>();
        return 50;
    }
    
    template <typename ConnectionBase>
    std::size_t operator()(
        ConnectionBase &,
        artifacts::header_max_size const &a) {
        debug<std::decay_t<decltype(a)>>();
        return 200;
    }
};

struct connection_manager {
public:
    using connection_base_type = connection_base<connection_manager>;
    
    riot::server::simple::security_policy security_policy;
    riot::server::simple::artifact_provider artifact_provider;
    std::list<std::weak_ptr<connection_base_type>> connections;
    
    // now the Boost-specific design
    boost::asio::io_context &io_ctx;
    std::unique_ptr<boost::asio::io_context::work> work;
    
    connection_manager(boost::asio::io_context &io_ctx_):
        io_ctx{io_ctx_} {
        work = std::make_unique<boost::asio::io_context::work>(io_ctx);
    }
    
    template <typename F>
    void post(F &&f) {
        boost::asio::post(io_ctx, [_f = std::move(f)] {
            _f();
        });
    }
};

}

#endif // RIOT_SERVER_SIMPLE_CONNECTION_MANAGER_INCLUDED
