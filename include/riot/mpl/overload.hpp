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
        struct overload_t {
            F1 f1;
            overload_t<Fs...> frest;

            template <typename ...Args>
            constexpr auto operator()(Args && ...args) const & {
                static_assert(is_callable_v<overload_t<F1, Fs...>, Args...>,
                    "no viable overload found");
                return _impl(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto operator()(Args && ...args) & {
                static_assert(is_callable_v<overload_t<F1, Fs...>, Args...>,
                    "no viable overload found");
                return _impl(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto operator()(Args && ...args) && {
                static_assert(is_callable_v<overload_t<F1, Fs...>, Args...>,
                    "no viable overload found");
                return std::move(*this)._impl(std::forward<Args>(args)...);
            }

        private:
            template <typename ...Args>
            constexpr auto _impl(Args && ...args) const & {
                if constexpr (is_callable_v<F1, Args...>)
                    return f1(std::forward<Args>(args)...);
                else
                    return frest(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto _impl(Args && ...args) & {
                if constexpr (is_callable_v<F1, Args...>)
                    return f1(std::forward<Args>(args)...);
                else
                    return frest(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto _impl(Args && ...args) && {
                if constexpr (is_callable_v<F1, Args...>)
                    return std::move(f1)(std::forward<Args>(args)...);
                else
                    return std::move(frest)(std::forward<Args>(args)...);
            }
        };

        template <typename F1>
        struct overload_t<F1> {
            F1 f1;

            template <typename ...Args>
            constexpr auto operator()(Args && ...args) const & {
                static_assert(is_callable_v<overload_t<F1>, Args...>,
                    "no viable overload found");
                return _impl(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto operator()(Args && ...args) & {
                static_assert(is_callable_v<overload_t<F1>, Args...>,
                    "no viable overload found");
                return _impl(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto operator()(Args && ...args) && {
                static_assert(is_callable_v<overload_t<F1>, Args...>,
                    "no viable overload found");
                return std::move(*this)._impl(std::forward<Args>(args)...);
            }

        private:
            template <typename ...Args>
            constexpr auto _impl(Args && ...args) const & {
                return f1(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto _impl(Args && ...args) & {
                return f1(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto _impl(Args && ...args) && {
                return std::move(f1)(std::forward<Args>(args)...);
            }
        };

        template <typename ...Fs>
        struct is_special_callable<overload_t<Fs...>> : std::true_type {};

        template <typename ...Fs, typename ...Args>
        struct is_special_callable_callable<overload_t<Fs...>, Args...> :
            std::integral_constant<
            bool, (is_callable<Fs, Args...>::value || ...)> {
            // a overload is callable provided that at least one of its callables 
            // contains an appropriate operator()
        };
    }

    template <typename F1>
    constexpr auto overload(F1 &&f1) {
        return detail::overload_t<std::decay_t<F1>>{static_cast<F1&&>(f1)};
    }

    template <typename F1, typename ...Frest>
    constexpr auto overload(F1 &&f1, Frest && ...frest) {
        return detail::overload_t<std::decay_t<F1>, std::decay_t<Frest>...>{
            static_cast<F1&&>(f1), overload(static_cast<Frest&&>(frest)...)};
    }
};

#endif // RIOT_MPL_OVERLOAD_HPP_INCLUDED
