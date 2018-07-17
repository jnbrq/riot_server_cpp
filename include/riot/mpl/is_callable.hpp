/**
 * @author Canberk Sönmez
 * 
 * @date Tue Jul 17 05:58:28 +03 2018
 * 
 */

#ifndef RIOT_MPL_IS_CALLABLE_HPP_INCLUDED
#define RIOT_MPL_IS_CALLABLE_HPP_INCLUDED

#include <type_traits>
#include <utility>

namespace riot::mpl {

namespace detail {
    // note to me: two-step checking is necessary, otherwise we have
    // circularities

    template <typename>
    struct is_special_callable: std::false_type {};

    template <typename, typename ...>
    struct is_special_callable_callable: std::false_type {};
}

template <typename T, typename ...Args>
struct is_callable {
private:
    template <typename C>
    static constexpr auto impl_(
        decltype( std::declval<C>() (std::declval<Args>() ...)) *) {
        return true;
    }

    template <typename>
    static constexpr auto impl_(...) {
        return false;
    }

    static constexpr auto check_() {
        if constexpr(detail::is_special_callable<T>::value) {
            return detail::is_special_callable_callable<T, Args...>::value;
        }
        else {
            return impl_<T>(0);
        }
    }
public:
    static constexpr bool value = check_();

    // mimic std::integral_constant
    typedef decltype(value) value_type;
    typedef is_callable type;
    constexpr operator value_type() const noexcept      { return value; }
    constexpr value_type operator()() const noexcept    { return value; }
};

template <typename T, typename ...Args>
constexpr bool is_callable_v = is_callable<T, Args...>::value;

}

#endif // RIOT_MPL_IS_CALLABLE_HPP_INCLUDED
