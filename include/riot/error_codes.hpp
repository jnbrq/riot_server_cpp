/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 */

#ifndef RIOT_ERROR_CODES_INCLUDED
#define RIOT_ERROR_CODES_INCLUDED

#include <limits>
#include <type_traits>

namespace riot {
enum protocol_error_code: unsigned short {
    /* no error case */
    err_no_error = 0,
    
    /* header errors */
    err_protocol = 5,
    err_malformed_header,
    err_no_name,
    err_activate_security_fail,
    
    /* parser errors */
    err_parser = 20,
    err_parser_regex,
    
    /* command errors */
    err_cmd = 40,
    err_cmd_not_impl,
    err_cmd_invalid_arg,
    
    err_cmd_cached_parser = 60,
    err_cmd_cached_parser_regex,
    
    /* security errors */
    err_security = 80,
    err_trigger_prohibited
};
}

#endif // RIOT_ERROR_CODES_INCLUDED
