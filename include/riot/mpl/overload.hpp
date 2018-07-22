/**
 * @author Canberk Sönmez
 * 
 * @date Tue Jul 17 05:58:28 +03 2018
 * 
 */

#ifndef RIOT_MPL_OVERLOAD_HPP_INCLUDED
#define RIOT_MPL_OVERLOAD_HPP_INCLUDED

#include <riot/mpl/is_callable.hpp>

namespace riot::mpl {

namespace detail {
    // overload implementation
    template <typename F1, typename ...Fs>
    struct overload {
        constexpr overload(F1 &&f1, Fs && ...fs) :
            f1_{ std::forward<F1>(f1) }, frest_{std::forward<Fs>(fs)...} {
        }

        template <typename ...Args>
        constexpr auto operator()(Args && ...args) const & {
            static_assert(is_callable_v<overload<F1, Fs...>, Args...>,
                            "no viable overload found");
            return _impl(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto operator()(Args && ...args) & {
            static_assert(is_callable_v<overload<F1, Fs...>, Args...>,
                            "no viable overload found");
            return _impl(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto operator()(Args && ...args) && {
            static_assert(is_callable_v<overload<F1, Fs...>, Args...>,
                            "no viable overload found");
            return std::move(*this)._impl(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto _impl(Args && ...args) const & {
            if constexpr (is_callable_v<F1, Args...>)
                return f1_(std::forward<Args>(args)...);
            else
                return frest_._impl(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto _impl(Args && ...args) & {
            if constexpr (is_callable_v<F1, Args...>)
                return f1_(std::forward<Args>(args)...);
            else
                return frest_._impl(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto _impl(Args && ...args) && {
            if constexpr (is_callable_v<F1, Args...>)
                return std::move(f1_)(std::forward<Args>(args)...);
            else
                return std::move(frest_)._impl(std::forward<Args>(args)...);
        }
    private:
        F1 f1_;
        overload<Fs...> frest_;
    };

    template <typename F1>
    struct overload<F1> {
        constexpr explicit overload(F1 &&f1) :
            f1_{ std::forward<F1>(f1) } {
        }

        template <typename ...Args>
        constexpr auto operator()(Args && ...args) const & {
            static_assert(is_callable_v<overload<F1>, Args...>,
                            "no viable overload found");
            return _impl(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto operator()(Args && ...args) & {
            static_assert(is_callable_v<overload<F1>, Args...>,
                            "no viable overload found");
            return _impl(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto operator()(Args && ...args) && {
            static_assert(is_callable_v<overload<F1>, Args...>,
                            "no viable overload found");
            return std::move(*this)._impl(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto _impl(Args && ...args) const & {
            return f1_(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto _impl(Args && ...args) & {
            return f1_(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        constexpr auto _impl(Args && ...args) && {
            return std::move(f1_)(std::forward<Args>(args)...);
        }
    private:
        F1 f1_;
    };

    template <typename ...Fs>
    struct is_special_callable<overload<Fs...>>: std::true_type {};

    template <typename ...Fs, typename ...Args>
    struct is_special_callable_callable<overload<Fs...>, Args...>:
        std::integral_constant<
            bool, ( is_callable<Fs, Args...>::value || ... )> {
        // a overload is callable provided that at least one of its callables 
        // contains an appropriate operator()
    };
}

using detail::overload;

};

#endif // RIOT_MPL_OVERLOAD_HPP_INCLUDED
