/**
 * @author Canberk Sönmez
 * 
 * @date Sat Jul 21 18:25:50 +03 2018
 * 
 */

#ifndef RIOT_SERVER_SPECIALIZED_CONNECTION_MANAGER_INCLUDED
#define RIOT_SERVER_SPECIALIZED_CONNECTION_MANAGER_INCLUDED

#include <riot/server/artifacts.hpp>
#include <riot/server/security_actions.hpp>

#include <riot/mpl/filtered_overload.hpp>

namespace riot::server {

namespace specialized {

// for more information, consider looking at example02.cpp

namespace fallback {
    
    // say the default policy is to simply raising warnings
    struct security_policy {
        template <typename ConnectionManager, typename SecurityAction>
        constexpr auto operator()(
            ConnectionManager &,
            connection_base<ConnectionManager> &,
            const SecurityAction &) {
            return security_actions::action_raise_warning_and_ignore;
        }
    };
    
    // the default artifact is simply returning true for bools and
    // sane numerical values
    struct artifact_provider {
        template <typename ConnectionManager, typename Artifact>
        constexpr std::enable_if_t<
                std::is_same_v<typename Artifact::result_type, bool>,
                typename Artifact::result_type
            > operator()(
                ConnectionManager &,
                connection_base<ConnectionManager> &,
                Artifact const &) {
            return true;
        }
        
        template <typename ConnectionManager, typename Artifact>
        constexpr std::enable_if_t<
                !std::is_same_v<typename Artifact::result_type, bool>,
                typename Artifact::result_type
            > operator()(
                ConnectionManager &,
                connection_base<ConnectionManager> &,
                Artifact const &) {
            return typename std::decay_t<Artifact>::result_type(0);
        }
        
        template <typename ConnectionManager>
        constexpr std::size_t operator()(
            ConnectionManager &,
            connection_base<ConnectionManager> &,
            artifacts::header_message_max_size const &) {
            return 50;
        }
        
        template <typename ConnectionManager>
        constexpr std::size_t operator()(
            ConnectionManager &,
            connection_base<ConnectionManager> &,
            artifacts::header_max_size const &) {
            return 200;
        }
    };
}

template <typename ...Args>
constexpr auto make_security_policy(Args && ...args) {
    return riot::mpl::filtered_overload(
        std::forward<Args>(args)...,
        fallback::security_policy{});
}

template <typename ...Args>
constexpr auto make_artifact_provider(Args && ...args) {
    return riot::mpl::filtered_overload(
        std::forward<Args>(args)...,
        fallback::artifact_provider{});
}

template <typename SecurityPolicy, typename ArtifactProvider>
struct connection_manager {
    connection_manager(
        boost::asio::io_context &io_ctx,
        SecurityPolicy &&security_policy,
        ArtifactProvider &&artifact_provider):
        io_ctx{io_ctx},
        security_policy_{std::forward<SecurityPolicy>(security_policy)},
        artifact_provider_{std::forward<ArtifactProvider>(artifact_provider)}{
    }
    
    using connection_base_type = connection_base<connection_manager>;
    
    template <typename S>
    constexpr auto security_policy(connection_base_type &connection, S &&s) {
        return security_policy_(*this, connection, std::forward<S>(s));
    }
    
    template <typename A>
    constexpr auto artifact_provider(connection_base_type &connection, A &&a) {
        return artifact_provider_(*this, connection, std::forward<A>(a));
    }
    
    std::list<std::weak_ptr<connection_base_type>> connections;
    
    boost::asio::io_context &io_ctx;
    
    template <typename F>
    void post(F &&f) {
        boost::asio::post(io_ctx, [_f = std::move(f)] {
            _f();
        });
    }
private:
    SecurityPolicy security_policy_;
    ArtifactProvider artifact_provider_;
};

}

}

#endif // RIOT_SERVER_SPECIALIZED_CONNECTION_MANAGER_INCLUDED
