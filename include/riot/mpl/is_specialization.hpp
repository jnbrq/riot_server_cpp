/**
 * @author Canberk Sönmez
 * 
 * @date Tue Jul 17 19:21:31 +03 2018
 * 
 */

#ifndef RIOT_MPL_IS_SPECIALIZATION_INCLUDED
#define RIOT_MPL_IS_SPECIALIZATION_INCLUDED

// check:
// https://stackoverflow.com/questions/16337610/how-to-know-if-a-type-is-a-\
// specialization-of-stdvector

#include <type_traits>

namespace riot::mpl {

template <typename T, template <typename...> typename Ref>
struct is_specialization: std::false_type {};

template <template <typename...> typename Ref, typename ...Args>
struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

template <typename T, template <typename...> typename Ref>
constexpr auto is_specialization_v = is_specialization<T, Ref>::value;

};

#endif // RIOT_MPL_IS_SPECIALIZATION_INCLUDED
