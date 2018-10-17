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
        template <typename ConnectionBase, typename SecurityAction>
        constexpr auto operator()(
            ConnectionBase &,
            const SecurityAction &) {
            return security_actions::action_raise_warning_and_ignore;
        }
    };
    
    // the default artifact is simply returning true for bools and
    // sane numerical values
    struct artifact_provider {
        template <typename ConnectionBase, typename Artifact>
        constexpr std::enable_if_t<
                std::is_same_v<typename Artifact::result_type, bool>,
                typename Artifact::result_type
            > operator()(
                ConnectionBase &,
                Artifact const &) {
            return true;
        }
        
        template <typename ConnectionBase, typename Artifact>
        constexpr std::enable_if_t<
                !std::is_same_v<typename Artifact::result_type, bool>,
                typename Artifact::result_type
            > operator()(
                ConnectionBase &,
                Artifact const &) {
            return typename std::decay_t<Artifact>::result_type(0);
        }
        
        template <typename ConnectionBase>
        constexpr std::size_t operator()(
            ConnectionBase &,
            artifacts::header_message_max_size<ConnectionBase> const &) {
            return 50;
        }
        
        template <typename ConnectionBase>
        constexpr std::size_t operator()(
            ConnectionBase &,
            artifacts::header_max_size<ConnectionBase> const &) {
            return 200;
        }
    };
}

// this part is unnecessary for my purposes
#if 0

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

#endif

template <
    typename ArtifactProvider,
    typename SecurityPolicy = fallback::security_policy,
    typename DataType = void*>
struct connection_manager {
    connection_manager(
        boost::asio::io_context &io_ctx,
        ArtifactProvider &&artifact_provider,
        SecurityPolicy &&security_policy = fallback::security_policy{},
        DataType = (void *) nullptr) :
        io_context{io_ctx},
        security_policy_{std::forward<SecurityPolicy>(security_policy)},
        artifact_provider_{std::forward<ArtifactProvider>(artifact_provider)} {
    }
    
    using connection_base_type = connection_base<connection_manager>;
    
    template <typename S>
    constexpr auto security_policy(connection_base_type &connection, S &&s) {
        return security_policy_(connection, std::forward<S>(s));
    }
    
    template <typename A>
    constexpr auto artifact_provider(connection_base_type &connection, A &&a) {
        return artifact_provider_(connection, std::forward<A>(a));
    }
    
    std::list<std::weak_ptr<connection_base_type>> connections;
    
    boost::asio::io_context &io_context;
private:
    SecurityPolicy security_policy_;
    ArtifactProvider artifact_provider_;
};

namespace detail {
    template <template <typename> typename What, typename F>
    struct case_of_t {
        template <typename _F>
        constexpr case_of_t(_F &&f) : f_{ std::forward<_F>(f) } {
        }

        template <typename ConnectionBase>
        constexpr auto operator()(
            ConnectionBase &cb,
            What<ConnectionBase> const & w) & {
            return f_(cb, w);
        }

        template <typename ConnectionBase>
        constexpr auto operator()(
            ConnectionBase &cb,
            What<ConnectionBase> const & w) const & {
            return f_(cb, w);
        }

        template <typename ConnectionBase>
        constexpr auto operator()(
            ConnectionBase &cb,
            What<ConnectionBase> const & w) && {
            return std::move(f_)(cb, w);
        }

        F f_;
    };
}

template <template <typename> typename What, typename F>
auto case_of(F &&f) {
    return detail::case_of_t<What, std::decay_t<F>>{std::forward<F>(f)};
}

}

}

#endif // RIOT_SERVER_SPECIALIZED_CONNECTION_MANAGER_INCLUDED
