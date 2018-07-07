/**
 * @author Canberk Sönmez
 * 
 * @date Fri Jul  6 20:14:08 +03 2018
 * 
 */

/**
 * Some references:
 *   - https://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/overview/core/line_based.html
 *   - https://stackoverflow.com/questions/41707028/can-you-set-a-byte-limit-for-asios-read-until
 */

#ifndef RIOT_SERVER_ASIO_HELPERS_INCLUDED
#define RIOT_SERVER_ASIO_HELPERS_INCLUDED

#include <boost/asio/read_until.hpp>
#include <type_traits>
#include <utility>

namespace riot::server {

namespace asio_helpers {

struct match_char_size_limited {
    match_char_size_limited(): c{'\n'}, sz{0} {
    }
    
    match_char_size_limited(char c_, std::size_t sz_, bool *exceeded):
        c{c_}, sz{sz_}, exceeded_{exceeded} {
    }
    
    char c;
    std::size_t sz;
    
    template <typename Iterator>
    std::pair<Iterator, bool> operator()(Iterator begin, Iterator end) {
        if (sz != 0 && end-begin > sz) {
            if (exceeded_ != nullptr)
                *exceeded_ = true;
            return std::make_pair(begin, true);
        }
        
        for (auto it = begin; it < end; ++it)
            if (*it == c)
                return std::make_pair(it, true);
        
        return std::make_pair(begin, false);
    }
private:
    bool *exceeded_{nullptr};
};

};

};

namespace boost::asio {

template <>
struct is_match_condition<riot::server::asio_helpers::match_char_size_limited>:
    public std::true_type {
};

};

#endif // RIOT_SERVER_ASIO_HELPERS_INCLUDED
