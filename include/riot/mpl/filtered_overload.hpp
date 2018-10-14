/**
 * @author Canberk Sönmez
 *
 * @date Tue Jul 17 05:58:28 +03 2018
 *
 */

#ifndef RIOT_MPL_FILTERED_OVERLOAD_HPP_INCLUDED
#define RIOT_MPL_FILTERED_OVERLOAD_HPP_INCLUDED

#include <riot/mpl/overload.hpp>

namespace riot::mpl {

    namespace detail {
        template <typename, typename ...>
        struct filter;

        template <typename>
        struct is_filter : std::false_type {};

        template <typename P, typename ...Fs>
        struct is_filter<filter<P, Fs...>> : std::true_type {};

        template <typename T>
        constexpr auto is_filter_v = is_filter<std::decay_t<T>>::value;

        // filtered_overload implementation

        template <typename F1, typename ...Frest>
        struct filtered_overload_t {
            // (1)
            // here is another approach: explicitly defining the constructor,
            // instead of {}-syntax.
            // (template parameter names needs to be different)
            // perfect forwarding done right, I wish.
            template <typename FF1, typename ...FFrest>
            constexpr filtered_overload_t(FF1 &&f1, FFrest && ...frest) :
                f1_{ std::forward<FF1>(f1) }, frest_{ std::forward<FFrest>(frest)... } {
            }

            // (2)
            template <typename ...Args>
            constexpr auto operator()(Args && ...args) const & {
                static_assert(is_callable_v<filtered_overload_t<F1, Frest...>, Args...>, "no viable overload found");
                return _impl(std::forward<Args>(args)...);
            }

            // (3)
            template <typename ...Args>
            constexpr auto operator()(Args && ...args) & {
                static_assert(is_callable_v<filtered_overload_t<F1, Frest...>, Args...>, "no viable overload found");
                return _impl(std::forward<Args>(args)...);
            }

            // (4)
            template <typename ...Args>
            constexpr auto operator()(Args && ...args) && {
                static_assert(is_callable_v<filtered_overload_t<F1, Frest...>, Args...>, "no viable overload found");
                return std::move(*this)._impl(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto _impl(Args && ...args) const & {
                // a filter object is also a callable
                if constexpr (is_callable_v<F1, Args...>) {
                    if constexpr (is_filter_v<F1>) {
                        if (f1_.check_condition(std::forward<Args>(args)...)) {
                            return f1_._impl(std::forward<Args>(args)...);
                        }
                        else {
                            return frest_._impl(std::forward<Args>(args)...);
                        }
                    }
                    else {
                        // not a filter, but callable; so use operator()
                        return f1_(std::forward<Args>(args)...);
                    }
                }
                else {
                    return frest_._impl(std::forward<Args>(args)...);
                }
            }

            template <typename ...Args>
            constexpr auto _impl(Args && ...args) & {
                if constexpr (is_callable_v<F1, Args...>) {
                    if constexpr (is_filter_v<F1>) {
                        if (f1_.check_condition(std::forward<Args>(args)...)) {
                            return f1_._impl(std::forward<Args>(args)...);
                        }
                        else {
                            return frest_._impl(std::forward<Args>(args)...);
                        }
                    }
                    else {
                        // not a filter, but callable; so use operator()
                        return f1_(std::forward<Args>(args)...);
                    }
                }
                else {
                    return frest_._impl(std::forward<Args>(args)...);
                }
            }

            template <typename ...Args>
            constexpr auto _impl(Args && ...args) && {
                if constexpr (is_callable_v<F1, Args...>) {
                    if constexpr (is_filter_v<F1>) {
                        if (std::move(f1_).check_condition(std::forward<Args>(args)...)) {
                            return std::move(f1_)._impl(std::forward<Args>(args)...);
                        }
                        else {
                            return std::move(frest_)._impl(std::forward<Args>(args)...);
                        }
                    }
                    else {
                        // not a filter, but callable; so use operator()
                        return std::move(f1_)(std::forward<Args>(args)...);
                    }
                }
                else {
                    return std::move(frest_)._impl(std::forward<Args>(args)...);
                }
            }

            F1 f1_;
            filtered_overload_t<Frest...> frest_;
        };

        template <typename F1>
        struct filtered_overload_t<F1> {
            // (1)
            template <typename FF1>
            constexpr explicit filtered_overload_t(FF1 &&f1) :
                f1_{ std::forward<FF1>(f1) } {
            }

            // (2)
            template <typename ...Args>
            constexpr auto operator()(Args && ...args) const & {
                // please note that the static assertion above also implies that is_filter_v<F1> is false!
                // it also states that f1_(args...) is valid, hence we can remove all the if branches
                static_assert(is_callable_v<filtered_overload_t<F1>, Args...>, "no viable overload found");
                return _impl(std::forward<Args>(args)...);
            }

            // (3)
            template <typename ...Args>
            constexpr auto operator()(Args && ...args) & {
                static_assert(is_callable_v<filtered_overload_t<F1>, Args...>, "no viable overload found");
                return _impl(std::forward<Args>(args)...);
            }

            // (4)
            template <typename ...Args>
            constexpr auto operator()(Args && ...args) && {
                static_assert(is_callable_v<filtered_overload_t<F1>, Args...>, "no viable overload found");
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

            F1 f1_;
        };

        template <typename ...Fs>
        struct is_special_callable<filtered_overload_t<Fs...>> : std::true_type {};

        template <typename ...Fs, typename ...Args>
        struct is_special_callable_callable<filtered_overload_t<Fs...>, Args...> :
            std::integral_constant<bool, ((!is_filter_v<Fs> && is_callable_v<Fs, Args...>) || ...)> {
            // a filtered_overload is callable provided that at least one of its non-filter callables
            // contain an appropriate operator()

            // please note that a filter can be callable, usually, and it works just like a
            // overload. however, its behaviour in filtered_overload is different!
        };

        template <typename P, typename ...Fs>
        struct filter : filtered_overload_t<Fs...> {
            template <typename PP, typename ...FFs>
            constexpr filter(PP &&p, FFs && ...fs) :
                filtered_overload_t<Fs...>{ std::forward<FFs>(fs)... }, p_{ std::forward<PP>(p) } {
            }

            using filtered_overload_t<Fs...>::operator();

            template <typename ...Args>
            constexpr auto check_condition(Args && ...args) const & {
                return p_(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto check_condition(Args && ...args) & {
                return p_(std::forward<Args>(args)...);
            }

            template <typename ...Args>
            constexpr auto check_condition(Args && ...args) && {
                return std::move(p_)(std::forward<Args>(args)...);
            }
        private:
            P p_;
        };

        template <typename ...Fs>
        struct is_special_callable<filter<Fs...>> : std::true_type {};

        template <typename P, typename ...Fs, typename ...Args>
        struct is_special_callable_callable<filter<P, Fs...>, Args...> :
            is_special_callable_callable<filtered_overload_t<Fs...>, Args...> {};
    }

    template <typename ...Fs>
    constexpr auto filtered_overload(Fs && ...fs) {
        return detail::filtered_overload_t<std::decay_t<Fs>...>{static_cast<Fs&&>(fs)...};
    }

    template <typename P, typename ...Fs>
    constexpr auto filter(P && p, Fs && ...fs) {
        return detail::filter<std::decay_t<P>, std::decay_t<Fs>...>{
            static_cast<P&&>(p), static_cast<Fs&&>(fs)...};
    }

}

#endif // RIOT_MPL_FILTERED_OVERLOAD_HPP_INCLUDED
